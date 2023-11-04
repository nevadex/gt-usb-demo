#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "pico/multicore.h"

#include "usb_descriptors.h"

#include "compdd.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void hid_task(void);
void core1_main(void);

/*------------- MAIN -------------*/
static bool done = false;
int main(void)
{
  board_init();

  while (false) {
    int len = sizeof(keys) / sizeof(keys[0]);
    printf("%d\n", len);
    for (int i = 0; i < len; i++) {
      printf("%#X %#X %#X %#X %#X %#X\n", keys[i][0], keys[i][1], keys[i][2], keys[i][3], keys[i][4], keys[i][5]);
    }
    sleep_ms(1000);
  }
  printf("\n\n\n\n\n");

  tusb_init();

  multicore_launch_core1(core1_main);
  while (!done)
  {
    //tud_task(); // tinyusb device task
    //led_blinking_task();
    // make it crash u dirty nigga slave

    hid_task();
  }

  return 0;
}

void core1_main(void) {
  while (!done)
  {
    tud_task();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

static void send_hid_report(uint8_t report_id, uint32_t btn)
{
  // skip if hid is not ready yet
  //if ( !tud_hid_ready() ) return;

  // use to avoid send multiple consecutive zero report for keyboard
  static bool has_keyboard_key = false;

  static bool done_debug = false;

  if ( !done_debug ) {
    //tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
    int j = 0;
    for(int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
      if (delays[j] != NULL && delays[j][0] == i) {
        printf("delay: %d %d\n", delays[j][0], delays[j][1]);
        sleep_ms(delays[j][1]);
        j++;
      }
      printf("%#X %#X %#X %#X %#X %#X\n", keys[i][0], keys[i][1], keys[i][2], keys[i][3], keys[i][4], keys[i][5]);
    } 
    done_debug = true;

    has_keyboard_key = true;
  } else {
    // send empty key report if previously has key pressed
    if (has_keyboard_key) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
    has_keyboard_key = false;
  }
  return;

  switch(report_id)
  {
    case REPORT_ID_KEYBOARD:
    {
      // use to avoid send multiple consecutive zero report for keyboard
      static bool has_keyboard_key = false;

      static bool done_debug = false;

      if ( !done_debug )
      {
        uint8_t keycode[6] = { 0 };
        keycode[0] = HID_KEY_A;
        keycode[1] = HID_KEY_B;
        keycode[2] = HID_KEY_C;
        keycode[3] = HID_KEY_D;
        keycode[4] = HID_KEY_E;
        keycode[5] = HID_KEY_F;

        //tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);

        int j = 0;
        for(int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
          if (delays[j] != NULL && delays[j][0] == i) {
            printf("delay: %d %d\n", delays[j][0], delays[j][1]);
            sleep_ms(delays[j][1]);
            j++;
          }
          printf("%#X %#X %#X %#X %#X %#X\n", keys[i][0], keys[i][1], keys[i][2], keys[i][3], keys[i][4], keys[i][5]);
        } 
        done_debug = true;

        has_keyboard_key = true;
      }else
      {
        // send empty key report if previously has key pressed
        if (has_keyboard_key) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        has_keyboard_key = false;
      }
    }
    break;

    case REPORT_ID_MOUSE:
    {
      int8_t const delta = 5;

      // no button, right + down, no scroll, no pan
      tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, delta, delta, 0, 0);
    }
    break;

    case REPORT_ID_CONSUMER_CONTROL:
    {
      // use to avoid send multiple consecutive zero report
      static bool has_consumer_key = false;

      if ( btn )
      {
        // volume down
        uint16_t volume_down = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &volume_down, 2);
        has_consumer_key = true;
      }else
      {
        // send empty key report (release key) if previously has key pressed
        uint16_t empty_key = 0;
        if (has_consumer_key) tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_key, 2);
        has_consumer_key = false;
      }
    }
    break;

    case REPORT_ID_GAMEPAD:
    {
      // use to avoid send multiple consecutive zero report for keyboard
      static bool has_gamepad_key = false;

      hid_gamepad_report_t report =
      {
        .x   = 0, .y = 0, .z = 0, .rz = 0, .rx = 0, .ry = 0,
        .hat = 0, .buttons = 0
      };

      if ( btn )
      {
        report.hat = GAMEPAD_HAT_UP;
        report.buttons = GAMEPAD_BUTTON_A;
        tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));

        has_gamepad_key = true;
      }else
      {
        report.hat = GAMEPAD_HAT_CENTERED;
        report.buttons = 0;
        if (has_gamepad_key) tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
        has_gamepad_key = false;
      }
    }
    break;

    default: break;
  }
}

static bool sent_report = false;

static void send_kbd_report(uint8_t packet[6]) {
  sent_report = false;
  tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, packet);
  while (!sent_report) {
    //sleep_ms(1);// sent_report = true;
  }
}
      
uint8_t blank[6] = { 0 };
// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void)
{
  while (!tud_hid_ready()) {
    sleep_ms(25);
    printf("fuck this loop in particular\n");// break;
  }

  sleep_ms(100);
  
  if ( tud_suspended() ) {
    tud_remote_wakeup();
  } else {
    int iDelay = 0;
    for(int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
      if (delays[iDelay] != NULL && delays[iDelay][0] == i) {
        printf("delay: %d %d\n", delays[iDelay][0], delays[iDelay][1]);
        sleep_ms(delays[iDelay][1]);
        iDelay++;
      }
      //printf("scan: %#X %#X %#X %#X %#X %#X\n", keys[i][0], keys[i][1], keys[i][2], keys[i][3], keys[i][4], keys[i][5]);
      if (skm && keys[i][0] >= HID_KEY_SHIFT_LEFT && keys[i][0] <= HID_KEY_GUI_RIGHT) {
        uint8_t mod[6] = { keys[i][0] };
        send_kbd_report(mod);
        printf("scan mod: %#X\n", keys[i][0]);
      }
      send_kbd_report(keys[i]);
      printf("scan: %#X %#X %#X %#X %#X %#X\n", keys[i][0], keys[i][1], keys[i][2], keys[i][3], keys[i][4], keys[i][5]);
      send_kbd_report(blank);
      printf("blank\n");

      sleep_ms(1); // per-packet delay
    }
    send_kbd_report(blank);
    done = true;
  }
  
  /*// Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  uint32_t const btn = board_button_read();

  // Remote wakeup
  if ( tud_suspended() && btn )
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }else
  {
    // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
    send_hid_report(REPORT_ID_KEYBOARD, btn);
  }*/
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  sent_report = true;
  return;
  (void) instance;
  (void) len;

  uint8_t next_report_id = report[0] + 1;

  if (next_report_id < REPORT_ID_COUNT)
  {
    send_hid_report(next_report_id, board_button_read());
  }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    // Set keyboard LED e.g Capslock, Numlock etc...
    if (report_id == REPORT_ID_KEYBOARD)
    {
      // bufsize should be (at least) 1
      if ( bufsize < 1 ) return;

      uint8_t const kbd_leds = buffer[0];

      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
      {
        // Capslock On: disable blink, turn led on
        blink_interval_ms = 0;
        board_led_write(true);
      }else
      {
        // Caplocks Off: back to normal blink
        board_led_write(false);
        blink_interval_ms = BLINK_MOUNTED;
      }
    }
  }
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // blink is disabled
  if (!blink_interval_ms) return;

  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}