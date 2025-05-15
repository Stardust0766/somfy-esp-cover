#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <NVSRollingCodeStorage.h>
#include <SomfyRemote.h>

#define EMITTER_GPIO 2
#define CC1101_FREQUENCY 433.42f
#define COVER_POS_OPEN 1.0f
#define COVER_POS_CLOSED 0.0f
#define STATE_MACHINE_INTERVAL 250 // ms

namespace esphome
{
  namespace somfy_esp_cover
  {
    class SomfyESPCover : public cover::Cover, public Component
    {
      public:
        void setup() override;
        void loop() override;
        void dump_config() override;
        cover::CoverTraits get_traits() override;

        void set_nvs_name(const char *nvs_name) {strcpy(nvs_name_, nvs_name);};
        void set_nvs_key(const char *nvs_key) {strcpy(nvs_key_, nvs_key);};
        void set_remote_code(uint32_t remote_code) {remote_code_ = remote_code;};
        void set_open_duration(float open_duration) {open_duration_ = open_duration;};
        void set_close_duration(float close_duration) {close_duration_ = close_duration;};
        void set_my_position(float my_position) {my_position_ = my_position;};
        void set_closed_position(float closed_position) {closed_position_ = closed_position;};
        void set_half_closed_position(float half_closed_position) {half_closed_position_ = half_closed_position;};
        void set_invert_behavior(bool invert_behavior) {invert_behavior_ = invert_behavior;};
    
        void program(); 


      private:
        enum CoverState
        {
          COVER_STATE_IDLE,
          COVER_STATE_STOP,
          COVER_STATE_MOVING_UP,
          COVER_STATE_MOVING_DOWN,
          COVER_STATE_MOVING_UP_AND_STOP,
          COVER_STATE_MOVING_DOWN_AND_STOP
        };

        enum Linearization
        {
          LINEARIZE_NONE,
          LINEARIZE_1,
          LINEARIZE_1_2
        };

        SomfyRemote *remote;
        NVSRollingCodeStorage *storage;
        char name[32];
        char key[32];
        CoverState current_state;
        CoverState last_state;
        uint32_t last_millis;
        float open_duration;
        float close_duration;
        float closed_position;
        float half_closed_position;
        Linearization linearization;
        float dp_to_mp_a_1;
        float dp_to_mp_b_1;
        float dp_to_mp_a_2;
        float dp_to_mp_b_2;
        float mp_to_dp_a_1;
        float mp_to_dp_b_1;
        float mp_to_dp_a_2;
        float mp_to_dp_b_2;
        float my_position;
        bool supports_position;
        bool supports_my_position;
        float target_position;
        float current_position;
        bool first_call;

        void sendCC1101Command(Command command); 
        float fromDisplayPosition(float displayPosition);
        float toDisplayPosition(float motorPosition);
        void logInformation(const char* message);
        void logConfiguration();
        void setCoverState(float motorPosition, cover::CoverOperation coverOperation);
    
      
      protected:
        void control(const cover::CoverCall &call) override;

        char nvs_name_[32];
        char nvs_key_[32];
        uint32_t remote_code_;
        float open_duration_;
        float close_duration_; 
        float my_position_;
        float closed_position_;
        float half_closed_position_;
        bool invert_behavior_;
    };

  } // namespace somfy_esp_cover
} // namespace esphome