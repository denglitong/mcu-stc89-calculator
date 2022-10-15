//
// Created by Litong Deng on 2022/10/14.
//

#include <8051.h>

#define ADDR_0 P1_0
#define ADDR_1 P1_1
#define ADDR_2 P1_2
#define ADDR_3 P1_3
#define EN_LED P1_4

#define KEY_IN_4 P2_7
#define KEY_IN_3 P2_6
#define KEY_IN_2 P2_5
#define KEY_IN_1 P2_4

#define KEY_OUT_1 P2_3
#define KEY_OUT_2 P2_2
#define KEY_OUT_3 P2_1
#define KEY_OUT_4 P2_0

#define LED_LINE P0
#define DOT 10

#define ADD 'A'
#define MINUS 'B'
#define MULTIPLE 'C'
#define DIVIDE 'D'
#define EQUAL 'E'
#define CLEAR 'F'

unsigned char KEY_STATUS[4][4] = {
    {1, 1, 1, 1},
    {1, 1, 1, 1},
    {1, 1, 1, 1},
    {1, 1, 1, 1},
};

unsigned char PREV_KEY_STATUS[4][4] = {
    {1, 1, 1, 1},
    {1, 1, 1, 1},
    {1, 1, 1, 1},
    {1, 1, 1, 1},
};

unsigned char KEY_BUFFER[4][4] = {
    {0xFF, 0xFF, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xFF, 0xFF},
};

unsigned char LED_CHAR[] = {
    0xff ^ 0b111111,  0xff ^ 0b110,     0xff ^ 0b1011011, 0xff ^ 0b1001111,
    0xff ^ 0b1100110, 0xff ^ 0b1101101, 0xff ^ 0b1111101, 0xff ^ 0b111,
    0xff ^ 0b1111111, 0xff ^ 0b1101111,
};

unsigned char LED_BUFF[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

void enable_u3_74hc138() {
  // 74HC138 芯片的使能引脚，G1 高电平 G2 低电平 才能启动74HC138的 3-8 译码电路
  ADDR_3 = 1; // G1 高电平
  EN_LED = 0; // G2低电平（G2A, G2B）
}

// i: 0 - (TUBE_SIZE-1)
void enable_tube(unsigned char i) {
  // P1_2 P1_1 P1_0
  // TUBE 0 000
  // TUBE 1 001
  // TUBE 2 010
  // TUBE 3 011
  // TUBE 4 100
  // TUBE 5 101
  P1 &= 1 << 3;
  P1 |= i;
}

void show_digit(unsigned char i) {
  // P0 = 0xff ^ digit_seg(i);
  // use array buffer to accelerate since the value is not changed in run-time.
  P0 = LED_CHAR[i];
}

unsigned char KeyCodeMap[4][4] = {{0x31, 0x32, 0x33, 0x26},
                                  {0x34, 0x35, 0x36, 0x25},
                                  {0x37, 0x38, 0x39, 0x28},
                                  {0x30, 0x1B, 0x0D, 0x27}};

/**
 * map 4 * 4 input keys to [0-9A-F]
 * keyboard layout:
 *  # # #     #
 *  # # #   #   #
 *  # # #     #
 *  #       #   #
 * @param key_row
 * @param key_col
 * @return
 */
unsigned char key_digit_map(unsigned char key_row, unsigned char key_col) {
  static unsigned char key_code_map[4][4] = {
      {1, 2, 3, ADD},
      {4, 5, 6, MULTIPLE},
      {7, 8, 9, MINUS},
      {0, CLEAR, EQUAL, DIVIDE},
  };
  return key_code_map[key_row][key_col];
}

void update_led_buffer(unsigned long digit) {
  signed char i = 0;
  unsigned char buf[6];
  for (i = 0; i < 6; ++i) {
    buf[i] = digit % 10;
    digit /= 10;
  }

  for (i = 5; i >= 1; i--) {
    if (buf[i] == 0) {
      LED_BUFF[i] = 0xFF;
    } else {
      break;
    }
  }
  for (; i >= 0; i--) {
    LED_BUFF[i] = LED_CHAR[buf[i]];
  }
}

void react_to_input_digit(unsigned char input_key) {
  // 结果
  static unsigned long RESULT_1 = 0, RESULT_2 = 0;
  // + - * / 操作数
  static unsigned long OPERAND = 0;
  // 操作符缓存
  static unsigned char OPERATOR_BUFF = 0;
  switch (input_key) {
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
    OPERAND = OPERAND * 10 + input_key;
    update_led_buffer(OPERAND);
    break;
  case ADD:
    RESULT_1 = OPERAND;
    OPERAND = 0;
    OPERATOR_BUFF = ADD;
    update_led_buffer(RESULT_1);
    break;
  case MINUS:
    RESULT_1 = OPERAND;
    OPERAND = 0;
    OPERATOR_BUFF = MINUS;
    update_led_buffer(RESULT_1);
    break;
  case MULTIPLE:
    RESULT_2 = OPERAND;
    OPERAND = 0;
    OPERATOR_BUFF = MULTIPLE;
    update_led_buffer(RESULT_2);
    break;
  case DIVIDE:
    RESULT_2 = OPERAND;
    OPERAND = 0;
    OPERATOR_BUFF = DIVIDE;
    update_led_buffer(RESULT_2);
    break;
  case EQUAL:
    if (OPERATOR_BUFF == ADD) {
      RESULT_1 += OPERAND;
      update_led_buffer(RESULT_1);
    }
    if (OPERATOR_BUFF == MINUS) {
      RESULT_1 -= OPERAND;
      update_led_buffer(RESULT_1);
    }
    if (OPERATOR_BUFF == MULTIPLE) {
      RESULT_2 *= OPERAND;
      update_led_buffer(RESULT_2);
    }
    if (OPERATOR_BUFF == DIVIDE) {
      RESULT_2 /= OPERAND;
      update_led_buffer(RESULT_2);
    }
    OPERAND = 0;
    break;
  case CLEAR:
    RESULT_1 = 0;
    RESULT_2 = 0;
    OPERAND = 0;
    update_led_buffer(0);
    break;
  default:
    break;
  }
}

_Noreturn void get_matrix_input_key_with_interrupt() {
  enable_u3_74hc138();

  EA = 1;  // enable global interrupt
  ET0 = 1; // enable Timer0 interrupt

  // setup T0_M1 = 0, T0_M0 = 1 (Timer0 mode TH0-TL0 16 bits timer)
  TMOD = 0x01;
  // setup TH0 TL0 initial value
  TH0 = 0xFC;
  TL0 = 0x67;
  TR0 = 1; // start/enable Timer0

  LED_BUFF[0] = LED_CHAR[0];

  while (1) {
    unsigned char i = 0, j = 0, digit = 0;
    for (i = 0; i < 4; ++i) {
      for (j = 0; j < 4; ++j) {
        if (PREV_KEY_STATUS[i][j] != KEY_STATUS[i][j]) {
          // 这里结合 i, j 就可以知道是弹起了哪个按键
          if (PREV_KEY_STATUS[i][j] != 0) {
            digit = key_digit_map(i, j);
            react_to_input_digit(digit);
          }
          PREV_KEY_STATUS[i][j] = KEY_STATUS[i][j];
        }
      }
    }
  }
}

void scan_keyboard() {
  static unsigned char keyout = 0;

  KEY_BUFFER[keyout][0] = (KEY_BUFFER[keyout][0] << 1) | KEY_IN_1;
  KEY_BUFFER[keyout][1] = (KEY_BUFFER[keyout][1] << 1) | KEY_IN_2;
  KEY_BUFFER[keyout][2] = (KEY_BUFFER[keyout][2] << 1) | KEY_IN_3;
  KEY_BUFFER[keyout][3] = (KEY_BUFFER[keyout][3] << 1) | KEY_IN_4;

  for (unsigned char i = 0; i < 4; ++i) {
    if (KEY_BUFFER[keyout][i] == 0x00) {
      KEY_STATUS[keyout][i] = 0;
    } else if (KEY_BUFFER[keyout][i] == 0xFF) {
      KEY_STATUS[keyout][i] = 1;
    }
  }

  keyout++;
  keyout = keyout & 0x03;

  switch (keyout) {
  case 0:
    KEY_OUT_1 = 0; // enable current keyout
    KEY_OUT_4 = 1; // disable previous keyout
    break;
  case 1:
    KEY_OUT_2 = 0;
    KEY_OUT_1 = 1;
    break;
  case 2:
    KEY_OUT_3 = 0;
    KEY_OUT_2 = 1;
    break;
  case 3:
    KEY_OUT_4 = 0;
    KEY_OUT_3 = 1;
    break;
  default:
    break;
  }
}

void turn_off_all_segs() { P0 = 0xff; }

void flush_led_buffer() {
  turn_off_all_segs();
  static unsigned char TUBE_IDX = 0;
  switch (TUBE_IDX) {
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
    enable_tube(TUBE_IDX);
    P0 = LED_BUFF[TUBE_IDX];
    TUBE_IDX++;
    break;
  case 5:
    enable_tube(TUBE_IDX);
    P0 = LED_BUFF[TUBE_IDX];
    TUBE_IDX = 0;
    break;
  default:
    break;
  }
}

void InterruptTime0_key() __interrupt(1) {
  // setup TH0 TL0 initial value, each interrupt(Timer0 overflow) will pass 1ms
  TH0 = 0xFC;
  TL0 = 0x67;

  // 再次的，状态的检测/展示和控制的更新/控制 分离
  // 1.在中断里面检测输入，但是在 main() 里面响应输入的事件
  scan_keyboard();
  // 2.在响应事件里面更新 LED_BUFF，但是刷新确是在每个中断事件里面
  flush_led_buffer();
}

// Build command:
// /usr/local/bin/sdcc --model-large src/main.c && stcgal -P stc89 -p
// /dev/tty.wchusbserial14220 main.ihx
int main() {
  get_matrix_input_key_with_interrupt();
  return 0;
}