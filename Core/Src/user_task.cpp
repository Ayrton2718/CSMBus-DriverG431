#include "user_task.h"
#include "can_csmbus/can_csmbus.hpp"

extern "C" {


}

static CCType_bool_t UserTask_canCallback(CCReg_t reg, const uint8_t* data, size_t len);
static void UserTask_resetCallback(void);
static void UserTask_timerCallback(void);

static bool g_rst_flg;
static CCTimer_t g_tim;

void UserTask_setup(void)
{
    g_rst_flg = false;
    
    CCTimer_start(&g_tim);
    CCIo_bind(CCType_appid_UNKNOWN, UserTask_canCallback, UserTask_resetCallback);
    CCTimer_bind(UserTask_timerCallback);
}

void UserTask_loop(void)
{
    uint32_t us = CCTimer_getUs(g_tim);
    if(1000 < us)
    {
        CCTimer_start(&g_tim);
    }

    if(g_rst_flg)
    {
        g_rst_flg = false;
    }
}

void UserTask_unsafeLoop(void)
{
    UserTask_loop();
}

static void UserTask_timerCallback(void)
{
}

static CCType_bool_t UserTask_canCallback(CCReg_t reg, const uint8_t* data, size_t len)
{
    return CCTYPE_FALSE;
}

static void UserTask_resetCallback(void)
{
	g_rst_flg = 1;
}
