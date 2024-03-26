#include "ssd1306_libopencm3.h"
#include "libopencm3/stm32/i2c.h"
#include "string.h"
#include "stdint.h"
#include "libopencm3/stm32/gpio.h"
static uint8_t SSD1306_Buffer[SSD1306_BUFFER_SIZE];
static SSD1306_t SSD1306; /*Screen object*/


void HAL_Delay(int time){
    for (int i = 0; i < time; i++) {
        __asm__("nop");
    }
    return;
}

void ssd1306_Reset(void){
    /*Do nothing for i2c*/
}

void ssd1306_WriteCommand(uint32_t i2c, uint8_t byte){
    uint32_t reg32 __attribute__((unused));

    i2c_send_start(i2c);

    /*check if stuff is stuffing or not, I2C_SR1 will
    expand to address of i2c+xyz to give Stautus register 1
    and so on*/
    while (!((I2C_SR1(i2c) & I2C_SR1_SB)
		& (I2C_SR2(i2c) & (I2C_SR2_MSL | I2C_SR2_BUSY))));


    /* destination address */
    i2c_send_7bit_address(i2c, SSD1306_ADDRESS, I2C_WRITE);

	/* Waiting for address is transferred. */
    while (!(I2C_SR1(i2c) & I2C_SR1_ADDR));

    gpio_toggle(GPIOB, GPIO7);
    HAL_Delay(100000);
    gpio_toggle(GPIOB, GPIO7);
    HAL_Delay(100000);
    gpio_toggle(GPIOB,GPIO7);
    /* cleaning addr condition sequence whatever
    the fuck that means */
    reg32 = I2C_SR2(i2c);

    /* send that bitch */
    i2c_send_data(i2c, byte);


    i2c_send_stop(i2c);

}

void ssd1306_WriteData(uint32_t i2c, uint8_t *buffer, uint8_t length){
    uint32_t reg32 __attribute__((unused));

    i2c_send_start(i2c);

    /*check if stuff is stuffing or not, I2C_SR1 will
    expand to address of i2c+xyz to give Stautus register 1
    and so on*/
    while (!((I2C_SR1(i2c) & I2C_SR1_SB)
		& (I2C_SR2(i2c) & (I2C_SR2_MSL | I2C_SR2_BUSY))));

    /* destination address */
    i2c_send_7bit_address(i2c, SSD1306_ADDRESS, I2C_WRITE);


	/* Waiting for address is transferred. */
	while (!(I2C_SR1(i2c) & I2C_SR1_ADDR));


    /* cleaning addr condition sequence whatever
    the fuck that means */
    reg32 = I2C_SR2(i2c);
    for (size_t i = 0; i < length; i++){
        /* send that bitch */
        i2c_send_data(i2c, buffer[i]);

        while (!(I2C_SR1(i2c) & I2C_SR1_BTF)); /* Await ByteTransferedFlag. */
    }
    i2c_send_stop(i2c);
}

void ssd1306_SetDisplayOn(uint32_t i2c, uint8_t on) {
    uint8_t value;
    if (on) {
        value = 0xAF;   // Display on
        SSD1306.DisplayOn = 1;
    } else {
        value = 0xAE;   // Display off
        SSD1306.DisplayOn = 0;
    }
    ssd1306_WriteCommand(i2c, value);
}

void ssd1306_UpdateScreen(uint32_t i2c) {
    // Write data to each page of RAM. Number of pages
    // depends on the screen height:
    //
    //  * 32px   ==  4 pages
    //  * 64px   ==  8 pages
    //  * 128px  ==  16 pages
    for(uint8_t i = 0; i < SSD1306_HEIGHT/8; i++) {
        ssd1306_WriteCommand(i2c,0xB0 + i); // Set the current RAM page address.
        ssd1306_WriteCommand(i2c,0x00 + SSD1306_X_OFFSET_LOWER);
        ssd1306_WriteCommand(i2c,0x10 + SSD1306_X_OFFSET_UPPER);
        ssd1306_WriteData(i2c,&SSD1306_Buffer[SSD1306_WIDTH*i],SSD1306_WIDTH);
    }
}

void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        // Don't write outside the buffer
        return;
    }

    // Draw in the right color
    if(color == White) {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    } else {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

void ssd1306_Fill(SSD1306_COLOR color) {
    memset(SSD1306_Buffer, (color == Black) ? 0x00 : 0xFF, sizeof(SSD1306_Buffer));
}

void ssd1306_SetContrast(uint32_t i2c, uint8_t value) {
    const uint8_t kSetContrastControlRegister = 0x81;
    ssd1306_WriteCommand(i2c, kSetContrastControlRegister);
    ssd1306_WriteCommand(i2c, value);
}

void ssd1306_Init(uint32_t i2c){
    ssd1306_Reset();
    // wait for screen to boot
    HAL_Delay(1000);
    ssd1306_SetDisplayOn(i2c, 0); /*turn off the display*/
    ssd1306_WriteCommand(i2c, 0x20); //Set Memory Addressing Mode
    ssd1306_WriteCommand(i2c, 0x00); // 00b,Horizontal Addressing Mode; 01b,Vertical Addressing Mode;
                                // 10b,Page Addressing Mode (RESET); 11b,Invalid
    ssd1306_WriteCommand(i2c, 0xB0); //Set Page Start Address for Page Addressing Mode,0-7

    ssd1306_WriteCommand(i2c,0x00); //---set low column address
    ssd1306_WriteCommand(i2c,0x10); //---set high column address

    ssd1306_WriteCommand(i2c,0x40); //--set start line address - CHECK

    ssd1306_SetContrast(i2c,0xFF);

    ssd1306_WriteCommand(i2c,0xA1); //set segment remap
    //gpio_toggle(GPIOB, GPIO7);
    ssd1306_WriteCommand(i2c,0xA6);
    ssd1306_WriteCommand(i2c,0xFF);
    ssd1306_WriteCommand(i2c,0x3F); // idk what this is
    ssd1306_WriteCommand(i2c,0xDB); //--set vcomh
    ssd1306_WriteCommand(i2c,0x20); //0x20,0.77xVcc

    ssd1306_WriteCommand(i2c,0x8D); //--set DC-DC enable
    ssd1306_WriteCommand(i2c,0x14); //
    ssd1306_SetDisplayOn(i2c,1);

        // Clear screen
    ssd1306_Fill(Black);

    // Flush buffer to screen
    ssd1306_UpdateScreen(i2c);

    // Set default values for screen object
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;

    SSD1306.Initialized = 1;
}

uint8_t get_init_status(void){
    if(SSD1306.Initialized == 1){
        return 1;
    } else {
        return 0;
    }
}
