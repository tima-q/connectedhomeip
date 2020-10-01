/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

/**
 *    @file
 *      This file declares test entry point for CHIP over BLE code unit tests in Linux
 *
 */

#ifndef CHIP_TEST_CHIP_BLE_STACK_MGR_H
#define CHIP_TEST_CHIP_BLE_STACK_MGR_H

#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
int TestCHIPoBLEStackManager();
#endif // CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
#endif // CHIP_TEST_CHIP_BLE_STACK_MGR_H
