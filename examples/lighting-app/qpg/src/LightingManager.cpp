/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "LightingManager.h"
#include "qvCHIP.h"
#include <support/logging/CHIPLogging.h>

#include <math.h>

LightingManager LightingManager::sLight;

RgbColor_t LightingManager::HsvToRgb(HsvColor_t hsv)
{
    RgbColor_t rgb;
    unsigned char region, remainder, p, q, t;

    if (hsv.s == 0)
    {
        rgb.r = hsv.v;
        rgb.g = hsv.v;
        rgb.b = hsv.v;
        return rgb;
    }

    region = hsv.h / 43;
    remainder = (unsigned char)((hsv.h - (region * 43)) * 6); 

    p = (unsigned char)((hsv.v * (255 - hsv.s)) >> 8);
    q = (unsigned char)((hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8);
    t = (unsigned char)((hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8);

    switch (region)
    {
        case 0:
            rgb.r = hsv.v; rgb.g = t; rgb.b = p;
            break;
        case 1:
            rgb.r = q; rgb.g = hsv.v; rgb.b = p;
            break;
        case 2:
            rgb.r = p; rgb.g = hsv.v; rgb.b = t;
            break;
        case 3:
            rgb.r = p; rgb.g = q; rgb.b = hsv.v;
            break;
        case 4:
            rgb.r = t; rgb.g = p; rgb.b = hsv.v;
            break;
        default:
            rgb.r = hsv.v; rgb.g = p; rgb.b = q;
            break;
    }

    return rgb;
}

RgbColor_t LightingManager::XYToRgb(uint8_t Level, uint16_t currentX, uint16_t currentY)
{
    // convert xyY color space to RGB

    // https://www.easyrgb.com/en/math.php
    // https://en.wikipedia.org/wiki/SRGB
    // refer https://en.wikipedia.org/wiki/CIE_1931_color_space#CIE_xy_chromaticity_diagram_and_the_CIE_xyY_color_space

    // The currentX/currentY attribute contains the current value of the normalized chromaticity value of x/y.
    // The value of x/y shall be related to the currentX/currentY attribute by the relationship
    // x = currentX/65536
    // y = currentY/65536
    // z = 1-x-y

    RgbColor_t rgb;

    float x,y,z;
    float X,Y,Z;
    float r,g,b;

    x = ((float)currentX)/65535.0f;
    y = ((float)currentY)/65535.0f;

    z = 1.0f - x - y;

    // Calculate XYZ values

    // Y - given brightness in 0 - 1 range
    Y = ((float)Level)/ 254.0f;

    X = (Y / y) * x;
    Z = (Y / y) * z;

    //X, Y and Z input refer to a D65/2° standard illuminant.
    //sR, sG and sB (standard RGB) output range = 0 ÷ 255
    // convert XYZ to RGB - CIE XYZ to sRGB
    r = (X * 3.2410f) - (Y * 1.5374f) - (Z * 0.4986f);
    g =  - (X * 0.9692f) + (Y * 1.8760f) + (Z * 0.0416f);
    b = (X * 0.0556f) - (Y * 0.2040f) + (Z * 1.0570f);

    // apply gamma 2.2 correction
    r = r <= 0.00304f ? 12.92f * r : (1.055f) * pow(r, (1.0f / 2.4f)) - 0.055f;
    g = g <= 0.00304f ? 12.92f * g : (1.055f) * pow(g, (1.0f / 2.4f)) - 0.055f;
    b = b <= 0.00304f ? 12.92f * b : (1.055f) * pow(b, (1.0f / 2.4f)) - 0.055f;

    //Round off
    r = r < 0 ? 0 : (r > 1 ? 1 : r);
    g = g < 0 ? 0 : (g > 1 ? 1 : g);
    b = b < 0 ? 0 : (b > 1 ? 1 : b);

    // these rgb values are in  the range of 0 to 1, convert to limit of HW specific LED
    rgb.r = (unsigned char)(r * 255);
    rgb.g = (unsigned char)(g * 255);
    rgb.b = (unsigned char)(b * 255);

    return rgb;
}

int LightingManager::Init()
{
    mState = kState_Off;
    mLevel = 64;
    mXY.x = 0; //TBD
    mXY.y = 0; //TBD
    mRGB = XYToRgb(mLevel, mXY.x, mXY.y);
    return 0;
}

bool LightingManager::IsTurnedOn()
{
    return mState == kState_On;
}

uint8_t LightingManager::GetLevel()
{
    return mLevel;
}

void LightingManager::SetCallbacks(LightingCallback_fn aActionInitiated_CB, LightingCallback_fn aActionCompleted_CB)
{
    mActionInitiated_CB = aActionInitiated_CB;
    mActionCompleted_CB = aActionCompleted_CB;
}

bool LightingManager::InitiateAction(Action_t aAction, int32_t aActor, uint16_t size, uint8_t * value)
{
    // TODO: this function is called InitiateAction because we want to implement some features such as ramping up here.
    bool action_initiated = false;
    State_t new_state;
    XyColor_t xy;

    switch (aAction)
    {
    case ON_ACTION:
        ChipLogProgress(NotSpecified, "LightMgr:ON: %s->ON", mState == kState_On ? "ON" : "OFF");
        break;
    case OFF_ACTION:
        ChipLogProgress(NotSpecified, "LightMgr:OFF: %s->OFF", mState == kState_On ? "ON" : "OFF");
        break;
    case LEVEL_ACTION:
        ChipLogProgress(NotSpecified, "LightMgr:LEVEL: lev:%u->%u", mLevel, *value);
        break;
    case COLOR_ACTION:
        xy = *(XyColor_t *)(value);
        ChipLogProgress(NotSpecified, "LightMgr:COLOR: xy:%u|%u->%u|%u", mXY.x, mXY.y, xy.x, xy.y);
        break;
    default:
        ChipLogProgress(NotSpecified, "LightMgr:Unknown");
        break;
    }

    // Initiate On/Off Action only when the previous one is complete.
    if (mState == kState_Off && aAction == ON_ACTION)
    {
        action_initiated = true;
        new_state        = kState_On;
    }
    else if (mState == kState_On && aAction == OFF_ACTION)
    {
        action_initiated = true;
        new_state        = kState_Off;
    }
    else if (aAction == LEVEL_ACTION && *value != mLevel)
    {
        action_initiated = true;
        if (*value == 0)
        {
            new_state = kState_Off;
        }
        else
        {
            new_state = kState_On;
        }
    }
    else if ((aAction == COLOR_ACTION) && (xy.x != mXY.x || xy.y != mXY.y))
    {
        action_initiated = true;
        if (xy.x == 0 && xy.y == 0) //TBC
        {
            new_state = kState_Off;
        }
        else
        {
            new_state = kState_On;
        }
    } 

    if (action_initiated)
    {
        if (mActionInitiated_CB)
        {
            mActionInitiated_CB(aAction);
        }
        if (aAction == LEVEL_ACTION)
        {
            SetLevel(*value);
        }
        else if (aAction == COLOR_ACTION)
        {
            SetColor(xy.x, xy.y);
        }
        else
        {
            Set(new_state == kState_On);
        }

        if (mActionCompleted_CB)
        {
            mActionCompleted_CB(aAction);
        }
    }

    return action_initiated;
}

void LightingManager::SetLevel(uint8_t aLevel)
{
    mLevel = aLevel;
    mRGB = XYToRgb(mLevel, mXY.x, mXY.y);
    UpdateLight();
}

void LightingManager::SetColor(uint16_t x, uint16_t y)
{
    mXY.x = x;
    mXY.y = y;
    mRGB = XYToRgb(mLevel, mXY.x, mXY.y);
    UpdateLight();
}

void LightingManager::Set(bool aOn)
{
    if (aOn)
    {
        mState = kState_On;
    }
    else
    {
        mState = kState_Off;
    }
    UpdateLight();
}

void LightingManager::UpdateLight()
{
    ChipLogProgress(NotSpecified, "UpdateLight: %d L:%d R:%d G:%d B:%d", mState, mLevel, mRGB.r, mRGB.g, mRGB.b);
    qvCHIP_PWMSetColor(mRGB.r, mRGB.g, mRGB.b);
    qvCHIP_PWMColorOnOff(mState == kState_On);
}
