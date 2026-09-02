#include "test_oled.h"
#include "oled.h"

void TestOLED_Hello(void)
{
    /* Test with small font at row 0 */
    OLED_DrawString("Hello World", 0, 12, &Font_11x18, 0, 1);


}
