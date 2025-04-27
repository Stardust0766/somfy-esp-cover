#include "esphome/core/log.h"
#include "somfy_esp_cover.h"

namespace esphome
{
  namespace somfy_esp_cover
  {
    static const char *TAG = "somfy_esp_cover.cover";

    void SomfyESPCover::setup()
    {
      //
      // Initialize the CC1101 wireless module
      //
      pinMode(EMITTER_GPIO, OUTPUT);
      digitalWrite(EMITTER_GPIO, LOW);

      ELECHOUSE_cc1101.Init();
      ELECHOUSE_cc1101.setMHZ(CC1101_FREQUENCY);


      //
      // Parsing the cover parameters
      //
      storage = new NVSRollingCodeStorage(nvs_name_, nvs_key_);
      remote = new SomfyRemote(EMITTER_GPIO, remote_code_, storage);
      
      strcpy (name, nvs_name_);
      strcpy (key, nvs_key_);
      
      // Validate open und close duration
      if (open_duration_ < 0.0 || close_duration_ < 0.0)
      {
        open_duration = -1.0;
        close_duration = -1.0;
        supports_position = false;
      }
      else
      {
        open_duration = open_duration_;
        close_duration = close_duration_;
        supports_position = true;
      }

      // Validate closed position offset
      if (closed_position_ < 0.0 || !supports_position) 
      {
        closed_position = -1.0;
      }
      else if (closed_position_ > 1.0)
      {
        closed_position = -1.0;
      }
      else
      {
        closed_position = closed_position_;
      }

      // Validate half closed position offset
      if (half_closed_position_ < 0.0 || closed_position < 0.0 || !supports_position) 
      {
        half_closed_position = -1.0;
      }
      else if (half_closed_position_ > 1.0)
      {
        half_closed_position = -1.0;
      }
      else
      {
        half_closed_position = half_closed_position_;
      }

      // Validate 'my' position
      if (my_position_ < 0.0 || my_position_ > 1.0 || !supports_position)
      {
        my_position = -1.0;
        supports_my_position = false;
      }
      else
      {
        my_position = my_position_;
        supports_my_position = true;
      }

      // Initialize linearization
      if (closed_position < 0.0)
      {
        linearization = LINEARIZE_NONE;
      }
      else if (closed_position >= 0.0 && half_closed_position < 0.0)
      {
        linearization = LINEARIZE_1;

        dp_to_mp_a_1 = 1.0 - closed_position;
        dp_to_mp_b_1 = closed_position;
        mp_to_dp_a_1 = 1.0 / (1 - closed_position);
        mp_to_dp_b_1 = 1.0 - 1.0 / (1 - closed_position);
      }
      else // closed_position >= 0.0 && half_closed_position >= 0.0)
      {
        linearization = LINEARIZE_1_2;
        
        dp_to_mp_a_1 = 2.0 * (half_closed_position - closed_position);
        dp_to_mp_b_1 = closed_position;
        mp_to_dp_a_1 = 1.0 / (2.0 * (half_closed_position - closed_position));
        mp_to_dp_b_1 = -closed_position / (2.0 * (half_closed_position - closed_position));
        dp_to_mp_a_2 = 2.0 * (1.0 - half_closed_position);
        dp_to_mp_b_2 = 1.0 - 2.0 * (1.0 - half_closed_position);
        mp_to_dp_a_2 = 1.0 / (2.0 * (1.0 - half_closed_position));
        mp_to_dp_b_2 = 1.0 - 1.0 / (2.0 * (1.0 - half_closed_position));
      }

      // Initialize state and position
      current_state = COVER_STATE_IDLE;
      target_position = COVER_POS_OPEN;
      current_position = COVER_POS_OPEN;

      // Initialize first call after constuction flag
      first_call = true;
    }


    void SomfyESPCover::setCoverState(float motorPosition, cover::CoverOperation coverOperation)
    {
      char message[255];

      sprintf(message, "Current motor position: %f", motorPosition);
      logInformation(message);
      
      if (supports_position)
      {
        position = toDisplayPosition(motorPosition);
        current_operation = coverOperation;
      }
      else
      {
        // Setting the end positions is required to get at least the icon to display the open and close state
        position = motorPosition;
      }
      
      publish_state();
    }


    void SomfyESPCover::loop()
    {
      // Get current time and calculate time difference
      uint32_t current_millis = millis();
      uint32_t millis_difference = current_millis - last_millis;

      if (millis_difference > STATE_MACHINE_INTERVAL)
      {
        // Get time difference to last execution in seconds
        float time_difference = millis_difference / 1000.0;

        // For the case that we do not change the next state variable we keep the current state
        CoverState next_state = current_state;

        // State machine
        switch (current_state)
        {
          default:
          case COVER_STATE_IDLE:
          {
            if (current_state != last_state)
            {
              setCoverState(current_position, cover::COVER_OPERATION_IDLE);
            }
            break;
          }

          case COVER_STATE_STOP:
          {
            logInformation("STOP");
            sendCC1101Command(Command::My);
            next_state = COVER_STATE_IDLE;
            logInformation("Set state to COVER_STATE_IDLE");
            break;
          }

          case COVER_STATE_MOVING_UP:
          {
            if (current_state != last_state)
            {
              logInformation("MOVE UP");
              sendCC1101Command(Command::Up);
            }
            if (supports_position)
            {
              current_position += time_difference / open_duration;
              if (current_position >= target_position)
              {
                current_position = target_position;
                next_state = COVER_STATE_IDLE;
                logInformation("Set state to COVER_STATE_IDLE");
              }
            }
            else
            {
              current_position = target_position;
              next_state = COVER_STATE_IDLE;
              logInformation("Set state to COVER_STATE_IDLE");
            }

            setCoverState(current_position, cover::COVER_OPERATION_OPENING);
            break;
          }

          case COVER_STATE_MOVING_UP_AND_STOP:
          {
            if (current_state != last_state)
            {
              logInformation("MOVE UP");
              sendCC1101Command(Command::Up);
            }
            if (supports_position)
            {
              current_position += time_difference / open_duration;
              if (current_position >= target_position)
              {
                current_position = target_position;
                next_state = COVER_STATE_STOP;
                logInformation("Set state to COVER_STATE_STOP");
              }
            }
            else
            {
              current_position = target_position;
              next_state = COVER_STATE_IDLE;
              logInformation("Set state to COVER_STATE_IDLE");
            }

            setCoverState(current_position, cover::COVER_OPERATION_OPENING);
            break;
          }

          case COVER_STATE_MOVING_DOWN:
          {
            if (current_state != last_state)
            {
              logInformation("MOVE DOWN");
              sendCC1101Command(Command::Down);
            }
            if (supports_position)
            {
              current_position -= time_difference / close_duration;
              if (current_position <= target_position)
              {
                current_position = target_position;
                next_state = COVER_STATE_IDLE;
                logInformation("Set state to COVER_STATE_IDLE");
              }
            }
            else
            {
              current_position = target_position;
              next_state = COVER_STATE_IDLE;
              logInformation("Set state to COVER_STATE_IDLE");
            }

            setCoverState(current_position, cover::COVER_OPERATION_CLOSING);
            break;
          }

          case COVER_STATE_MOVING_DOWN_AND_STOP:
          {
            if (current_state != last_state)
            {
              logInformation("MOVE DOWN");
              sendCC1101Command(Command::Down);
            }
            if (supports_position)
            {
              current_position -= time_difference / close_duration;
              if (current_position <= target_position)
              {
                current_position = target_position;
                next_state = COVER_STATE_STOP;
                logInformation("Set state to COVER_STATE_STOP");
              }
            }
            else
            {
              current_position = target_position;
              next_state = COVER_STATE_IDLE;
              logInformation("Set state to COVER_STATE_IDLE");
            }

            setCoverState(current_position, cover::COVER_OPERATION_CLOSING);
            break;
          }
        }

        // Store current state and time for next call
        last_state = current_state;
        current_state = next_state;
        last_millis = current_millis;
      }
    }

    void SomfyESPCover::dump_config()
    {
      ESP_LOGCONFIG(TAG, "Somfy ESP cover");
    }

    cover::CoverTraits SomfyESPCover::get_traits()
    {
      auto traits = cover::CoverTraits();
      traits.set_is_assumed_state(true);

      if (supports_position)
      {
        traits.set_supports_position(true);
      }
      else
      {
        traits.set_supports_position(false);
      }
      
      traits.set_supports_stop(true);
      
      traits.set_supports_toggle(false);
      traits.set_supports_tilt(false);
      
      return traits;
    }

    void SomfyESPCover::sendCC1101Command(Command command) 
    {
      ELECHOUSE_cc1101.SetTx();
      remote->sendCommand(command);
      ELECHOUSE_cc1101.setSidle();
    }

    float SomfyESPCover::fromDisplayPosition(float displayPosition)
    {
      float motor_position;
      if (supports_position)
      {
        if (displayPosition > 0.0)
        {
          switch (linearization)
          {
            case LINEARIZE_NONE:
            default:
            {
              motor_position = displayPosition;
              break;
            }

            case LINEARIZE_1:
            {
              motor_position = dp_to_mp_a_1 * displayPosition + dp_to_mp_b_1;
              break;
            }

            case LINEARIZE_1_2:
            {
              if (displayPosition < 0.5)
              {
                motor_position = dp_to_mp_a_1 * displayPosition + dp_to_mp_b_1;
              }
              else
              {
                motor_position = dp_to_mp_a_2 * displayPosition + dp_to_mp_b_2;
              }
              break;
            }
          }
      }
        else
        {
          motor_position = 0.0;
        }
      }
      else
      {
        motor_position = displayPosition;
      }

      if (motor_position < 0.0)
      {
        motor_position = 0.0;
      }
      if (motor_position > 1.0)
      {
        motor_position = 1.0;
      }

      return motor_position;
    }


    float SomfyESPCover::toDisplayPosition(float motorPosition)
    {
      float display_position;

      if (supports_position)
      {
        switch (linearization)
        {
          case LINEARIZE_NONE:
          default:
          {
            display_position = motorPosition;
            break;
          }

          case LINEARIZE_1:
          {
            display_position = mp_to_dp_a_1 * motorPosition + mp_to_dp_b_1;
            break;
          }

          case LINEARIZE_1_2:
          {
            if (motorPosition < half_closed_position)
            {
              display_position = mp_to_dp_a_1 * motorPosition + mp_to_dp_b_1;
            }
            else
            {
              display_position = mp_to_dp_a_2 * motorPosition + mp_to_dp_b_2;
            }
            break;
          }
        }
      }
      else
      {
        display_position = motorPosition;
      }

      if (display_position < 0.0)
      {
        display_position = 0.0;
      }
      if (display_position > 1.0)
      {
        display_position = 1.0;
      }

      return display_position;
    }

    void SomfyESPCover::logInformation(const char* message)
    {
      char esp_log_message[512];

      sprintf(esp_log_message, "<%s> - %s", key, message);
      ESP_LOGI(name, esp_log_message);
    }


    void SomfyESPCover::logConfiguration()
    {
      if (supports_position)
      {
        logInformation("Position support is enabled.");
      }
      else
      {
        logInformation("Position support is disabled.");
      }

      if (supports_my_position)
      {
        logInformation("'my' position support is enabled.");
      }
      else
      {
        logInformation("'my' position support is disabled.");
      }
      
      switch (linearization)
      {
        case LINEARIZE_NONE:
        default:
        {
          logInformation("Linearization is disabled.");
          break;
        }

        case LINEARIZE_1:
        {
          logInformation("Single segment (0.0-1.0) linearization enabled.");
          break;
        }

        case LINEARIZE_1_2:
        {
          logInformation("Dual segment (0.0-0.5 / 0.5-1.0) linearization enabled.");
          break;
        }
      }
    }

    
    void SomfyESPCover::control(const cover::CoverCall &call)
    {
      // Log configuration in first call 
      if (first_call)
      {
        logConfiguration();
        first_call = false;
      }

      // Evaluate demand position
      if (call.get_position().has_value()) 
      {
        float display_position = *call.get_position();

        if (display_position == COVER_POS_OPEN) 
        {
          target_position = COVER_POS_OPEN;
          current_state = COVER_STATE_MOVING_UP;
          logInformation("Set state to COVER_STATE_MOVING_UP");
        } 
        else if (display_position == COVER_POS_CLOSED) 
        {
          target_position = COVER_POS_CLOSED;
          current_state = COVER_STATE_MOVING_DOWN;
          logInformation("Set state to COVER_STATE_MOVING_DOWN");
        } 
        else 
        {
          if (supports_position)
          {
            // Convert from display position to real motor position (with offset)
            float motor_position = fromDisplayPosition(display_position);
            
            if (motor_position > current_position)
            {
              target_position = motor_position;
              current_state = COVER_STATE_MOVING_UP_AND_STOP;
              logInformation("Set state to COVER_STATE_MOVING_UP_AND_STOP");
            }
            else if (motor_position < current_position)
            {
              target_position = motor_position;
              current_state = COVER_STATE_MOVING_DOWN_AND_STOP;
              logInformation("Set state to COVER_STATE_MOVING_DOWN_AND_STOP");
            } 
            else
            {
              current_state = COVER_STATE_IDLE;
              logInformation("Set state to COVER_STATE_IDLE");
            } 
          }
          else
          {
            logInformation("Invalid position command received!");
          }
        }
      }


      // Stop button has been pressed
      if (call.get_stop()) 
      {
        if (current_state == COVER_STATE_IDLE)
        {
          if (supports_position)
          {
            if (supports_my_position)
            {
              // We simulate the 'my' position on our own. This has two advantages:
              // 1. There is no need to somehow synchronize the stored position between the cover and this implementation.
              // 2. There is one possibility less to get the real and the calculatated position out of sync.
              if (my_position > current_position)
              {
                target_position = my_position;
                current_state = COVER_STATE_MOVING_UP_AND_STOP;
                logInformation("Move cover to 'my' position. Set state to COVER_STATE_MOVING_UP_AND_STOP");
              }
              else if  (my_position < current_position)
              {
                target_position = my_position;
                current_state = COVER_STATE_MOVING_DOWN_AND_STOP;
                logInformation("Move cover to 'my' position. Set state to COVER_STATE_MOVING_DOWN_AND_STOP");
              }
              else
              {
                current_state = COVER_STATE_IDLE;
                logInformation("Cover is positioned at 'my' position. Set state to COVER_STATE_IDLE.");
              }
            }
          }
          else
          {
            current_state = COVER_STATE_STOP;
            logInformation("Set state to COVER_STATE_STOP");
          }
        }
        else
        {
          current_state = COVER_STATE_STOP;
          logInformation("Set state to COVER_STATE_STOP");
        }
      }
    }


    void SomfyESPCover::program() 
    {
      logInformation("PROGRAM");
      sendCC1101Command(Command::Prog);
    }
  } // namespace somfy_esp_cover
} // namespace esphome
