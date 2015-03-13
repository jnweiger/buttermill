#define LED_PORT PORTF
#define LED_DDR DDRF
#define GREEN DDF4
#define RED DDF6
#define YELLOW DDF5

// Function declarations
void led_init(void);
void green_on(void);
void green_off(void);
void red_on(void);
void red_off(void);
void yellow_on(void);
void yellow_off(void);
void lights_on(void);
void debug(uint8_t val);
