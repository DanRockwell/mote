#ifndef LTC2984_CFG_H
#define LTC2984_CFG_H


//**********************************************
// -- SENSOR TYPES --
//**********************************************
// RTD
#define SENSOR_TYPE__RTD_PT_10          0xa   << 27
#define SENSOR_TYPE__RTD_PT_50          0xb   << 27
#define SENSOR_TYPE__RTD_PT_100         0xc   << 27
#define SENSOR_TYPE__RTD_PT_200         0xd   << 27
#define SENSOR_TYPE__RTD_PT_500         0xe   << 27
#define SENSOR_TYPE__RTD_PT_1000        0xf   << 27
#define SENSOR_TYPE__RTD_PT_1000_375    0x10  << 27
#define SENSOR_TYPE__RTD_NI_120         0x11  << 27
#define SENSOR_TYPE__RTD_CUSTOM         0x12  << 27
// Sense Resistor
#define SENSOR_TYPE__SENSE_RESISTOR     0x1d  << 27
// -
#define SENSOR_TYPE__NONE               0x0   << 27
// Direct ADC
#define SENSOR_TYPE__DIRECT_ADC         0x1e  << 27
// Thermistor
#define SENSOR_TYPE__THERMISTOR_44004_2P252K_25C        0x13 << 27
#define SENSOR_TYPE__THERMISTOR_44005_3K_25C            0x14 << 27
#define SENSOR_TYPE__THERMISTOR_44007_5K_25C            0x15 << 27
#define SENSOR_TYPE__THERMISTOR_44006_10K_25C           0x16 << 27
#define SENSOR_TYPE__THERMISTOR_44008_30K_25C           0x17 << 27
#define SENSOR_TYPE__THERMISTOR_YSI_400_2P252K_25C      0x18 << 27
#define SENSOR_TYPE__THERMISTOR_1003K_1K_25C            0x19 << 27
#define SENSOR_TYPE__THERMISTOR_CUSTOM_STEINHART_HART   0x1a << 27
#define SENSOR_TYPE__THERMISTOR_CUSTOM_TABLE            0x1b << 27
// Thermocouple
#define SENSOR_TYPE__TYPE_J_THERMOCOUPLE 0x1 << 27
#define SENSOR_TYPE__TYPE_K_THERMOCOUPLE 0x2 << 27
#define SENSOR_TYPE__TYPE_E_THERMOCOUPLE 0x3 << 27
#define SENSOR_TYPE__TYPE_N_THERMOCOUPLE 0x4 << 27
#define SENSOR_TYPE__TYPE_R_THERMOCOUPLE 0x5 << 27
#define SENSOR_TYPE__TYPE_S_THERMOCOUPLE 0x6 << 27
#define SENSOR_TYPE__TYPE_T_THERMOCOUPLE 0x7 << 27
#define SENSOR_TYPE__TYPE_B_THERMOCOUPLE 0x8 << 27
#define SENSOR_TYPE__CUSTOM_THERMOCOUPLE 0x9 << 27
// Off-Chip Diode
#define SENSOR_TYPE__OFF_CHIP_DIODE     0x1c << 27

//**********************************************
// -- RTD --
//**********************************************
// rtd - rsense channel
#define RTD_RSENSE_CHANNEL_LSB 22
#define RTD_RSENSE_CHANNEL__NONE 0x0  << 22
#define RTD_RSENSE_CHANNEL__1    0x1  << 22
#define RTD_RSENSE_CHANNEL__2    0x2  << 22
#define RTD_RSENSE_CHANNEL__3    0x3  << 22
#define RTD_RSENSE_CHANNEL__4    0x4  << 22
#define RTD_RSENSE_CHANNEL__5    0x5  << 22
#define RTD_RSENSE_CHANNEL__6    0x6  << 22
#define RTD_RSENSE_CHANNEL__7    0x7  << 22
#define RTD_RSENSE_CHANNEL__8    0x8  << 22
#define RTD_RSENSE_CHANNEL__9    0x9  << 22
#define RTD_RSENSE_CHANNEL__10   0xa  << 22
#define RTD_RSENSE_CHANNEL__11   0xb  << 22
#define RTD_RSENSE_CHANNEL__12   0xc  << 22
#define RTD_RSENSE_CHANNEL__13   0xd  << 22
#define RTD_RSENSE_CHANNEL__14   0xe  << 22
#define RTD_RSENSE_CHANNEL__15   0xf  << 22
#define RTD_RSENSE_CHANNEL__16   0x10 << 22
#define RTD_RSENSE_CHANNEL__17   0x11 << 22
#define RTD_RSENSE_CHANNEL__18   0x12 << 22
#define RTD_RSENSE_CHANNEL__19   0x13 << 22
#define RTD_RSENSE_CHANNEL__20   0x14 << 22
// rtd - num wires
#define RTD_NUM_WIRES_LSB 20
#define RTD_NUM_WIRES__2_WIRE 0x0 << 20
#define RTD_NUM_WIRES__3_WIRE 0x1 << 20
#define RTD_NUM_WIRES__4_WIRE 0x2 << 20
#define RTD_NUM_WIRES__4_WIRE_KELVIN_RSENSE 0x3 << 20
// rtd - excitation mode
#define RTD_EXCITATION_MODE_LSB 18
#define RTD_EXCITATION_MODE__NO_ROTATION_NO_SHARING     0X0 << 18
#define RTD_EXCITATION_MODE__NO_ROTATION_SHARING        0X1 << 18
#define RTD_EXCITATION_MODE__ROTATION_SHARING           0X2 << 18
// rtd - excitation current
#define RTD_EXCITATION_CURRENT_LSB 14
#define RTD_EXCITATION_CURRENT__EXTERNAL 0X0 << 14
#define RTD_EXCITATION_CURRENT__5UA      0X1 << 14
#define RTD_EXCITATION_CURRENT__10UA     0X2 << 14
#define RTD_EXCITATION_CURRENT__25UA     0X3 << 14
#define RTD_EXCITATION_CURRENT__50UA     0X4 << 14
#define RTD_EXCITATION_CURRENT__100UA    0X5 << 14
#define RTD_EXCITATION_CURRENT__250UA    0X6 << 14
#define RTD_EXCITATION_CURRENT__500UA    0X7 << 14
#define RTD_EXCITATION_CURRENT__1MA      0X8 << 14
// rtd - standard
#define RTD_STANDARD_LSB 12
#define RTD_STANDARD__EUROPEAN 0X0  << 12
#define RTD_STANDARD__AMERICAN 0X1  << 12
#define RTD_STANDARD__JAPANESE 0X2  << 12
#define RTD_STANDARD__ITS_90   0X3  << 12
// rtd - custom address
#define RTD_CUSTOM_ADDRESS_LSB 6
// rtd - custom length-1
#define RTD_CUSTOM_LENGTH_1_LSB 0
// rtd - custom values
#define RTD_CUSTOM_VALUES_LSB 31

//**********************************************
// -- Sense Resistor --
//**********************************************
// sense resistor - value
#define SENSE_RESISTOR_VALUE_LSB 0

//**********************************************
// -- Direct ADC --
//**********************************************
// Direct ADC - differential?
#define DIRECT_ADC_DIFFERENTIAL_LSB 26
#define DIRECT_ADC_DIFFERENTIAL 0 << 26
#define DIRECT_ADC_SINGLE_ENDED 1 << 26

//**********************************************
// -- Thermistor --
//**********************************************
// thermistor - rsense channel
#define THERMISTOR_RSENSE_CHANNEL_LSB 22
#define THERMISTOR_RSENSE_CHANNEL__NONE 0x0  << 22
#define THERMISTOR_RSENSE_CHANNEL__1    0x1  << 22
#define THERMISTOR_RSENSE_CHANNEL__2    0x2  << 22
#define THERMISTOR_RSENSE_CHANNEL__3    0x3  << 22
#define THERMISTOR_RSENSE_CHANNEL__4    0x4  << 22
#define THERMISTOR_RSENSE_CHANNEL__5    0x5  << 22
#define THERMISTOR_RSENSE_CHANNEL__6    0x6  << 22
#define THERMISTOR_RSENSE_CHANNEL__7    0x7  << 22
#define THERMISTOR_RSENSE_CHANNEL__8    0x8  << 22
#define THERMISTOR_RSENSE_CHANNEL__9    0x9  << 22
#define THERMISTOR_RSENSE_CHANNEL__10   0xa  << 22
#define THERMISTOR_RSENSE_CHANNEL__11   0xb  << 22
#define THERMISTOR_RSENSE_CHANNEL__12   0xc  << 22
#define THERMISTOR_RSENSE_CHANNEL__13   0xd  << 22
#define THERMISTOR_RSENSE_CHANNEL__14   0xe  << 22
#define THERMISTOR_RSENSE_CHANNEL__15   0xf  << 22
#define THERMISTOR_RSENSE_CHANNEL__16   0x10 << 22
#define THERMISTOR_RSENSE_CHANNEL__17   0x11 << 22
#define THERMISTOR_RSENSE_CHANNEL__18   0x12 << 22
#define THERMISTOR_RSENSE_CHANNEL__19   0x13 << 22
#define THERMISTOR_RSENSE_CHANNEL__20   0x14 << 22
// thermistor - differential?
#define THERMISTOR_DIFFERENTIAL_LSB 21
#define THERMISTOR_DIFFERENTIAL 0x0 << 21
#define THERMISTOR_SINGLE_ENDED 0x1 << 21
// thermistor - excitation mode
#define THERMISTOR_EXCITATION_MODE_LSB 19
#define THERMISTOR_EXCITATION_MODE__NO_SHARING_NO_ROTATION 0x0 << 19
#define THERMISTOR_EXCITATION_MODE__SHARING_ROTATION       0x1 << 19
#define THERMISTOR_EXCITATION_MODE__SHARING_NO_ROTATION    0x2 << 19
// thermistor - excitation current
#define THERMISTOR_EXCITATION_CURRENT_LSB 15
#define THERMISTOR_EXCITATION_CURRENT__INVALID_SETTING1 0x0 << 15
#define THERMISTOR_EXCITATION_CURRENT__250NA            0x1 << 15
#define THERMISTOR_EXCITATION_CURRENT__500NA            0x2 << 15
#define THERMISTOR_EXCITATION_CURRENT__1UA              0x3 << 15
#define THERMISTOR_EXCITATION_CURRENT__5UA              0x4 << 15
#define THERMISTOR_EXCITATION_CURRENT__10UA             0x5 << 15
#define THERMISTOR_EXCITATION_CURRENT__25UA             0x6 << 15
#define THERMISTOR_EXCITATION_CURRENT__50UA             0x7 << 15
#define THERMISTOR_EXCITATION_CURRENT__100UA            0x8 << 15
#define THERMISTOR_EXCITATION_CURRENT__250UA            0x9 << 15
#define THERMISTOR_EXCITATION_CURRENT__500UA            0xa << 15
#define THERMISTOR_EXCITATION_CURRENT__1MA              0xb << 15
#define THERMISTOR_EXCITATION_CURRENT__AUTORANGE        0xc << 15
#define THERMISTOR_EXCITATION_CURRENT__INVALID_SETTING2 0xd << 15
#define THERMISTOR_EXCITATION_CURRENT__INVALID_SETTING3 0xe << 15
#define THERMISTOR_EXCITATION_CURRENT__EXTERNAL         0xf << 15
// thermistor - custom address
#define THERMISTOR_CUSTOM_ADDRESS_LSB 6
// thermistor - custom length-1
#define THERMISTOR_CUSTOM_LENGTH_1_LSB 0
// thermistor - custom values
#define THERMISTOR_CUSTOM_VALUES_LSB 31

//**********************************************
// -- Thermocouple --
//**********************************************
// tc - cold junction ch
#define TC_COLD_JUNCTION_CH_LSB 22
#define TC_COLD_JUNCTION_CH__NONE       0x0  << 22
#define TC_COLD_JUNCTION_CH__1          0x1  << 22
#define TC_COLD_JUNCTION_CH__2          0x2  << 22
#define TC_COLD_JUNCTION_CH__3          0x3  << 22
#define TC_COLD_JUNCTION_CH__4          0x4  << 22
#define TC_COLD_JUNCTION_CH__5          0x5  << 22
#define TC_COLD_JUNCTION_CH__6          0x6  << 22
#define TC_COLD_JUNCTION_CH__7          0x7  << 22
#define TC_COLD_JUNCTION_CH__8          0x8  << 22
#define TC_COLD_JUNCTION_CH__9          0x9  << 22
#define TC_COLD_JUNCTION_CH__10         0xa  << 22
#define TC_COLD_JUNCTION_CH__11         0xb  << 22
#define TC_COLD_JUNCTION_CH__12         0xc  << 22
#define TC_COLD_JUNCTION_CH__13         0xd  << 22
#define TC_COLD_JUNCTION_CH__14         0xe  << 22
#define TC_COLD_JUNCTION_CH__15         0xf  << 22
#define TC_COLD_JUNCTION_CH__16         0x10 << 22
#define TC_COLD_JUNCTION_CH__17         0x11 << 22
#define TC_COLD_JUNCTION_CH__18         0x12 << 22
#define TC_COLD_JUNCTION_CH__19         0x13 << 22
#define TC_COLD_JUNCTION_CH__20         0x14 << 22
// tc - differential?
#define TC_DIFFERENTIAL_LSB 21
#define TC_DIFFERENTIAL 0x0 << 21
#define TC_SINGLE_ENDED 0x1 << 21
// tc - open ckt detect?
#define TC_OPEN_CKT_DETECT_LSB 20
#define TC_OPEN_CKT_DETECT__NO  0x0 << 20
#define TC_OPEN_CKT_DETECT__YES 0x1 << 20
// tc - open ckt detect current
#define TC_OPEN_CKT_DETECT_CURRENT_LSB 18
#define TC_OPEN_CKT_DETECT_CURRENT__10UA  0x0 << 18
#define TC_OPEN_CKT_DETECT_CURRENT__100UA 0x1 << 18
#define TC_OPEN_CKT_DETECT_CURRENT__500UA 0x2 << 18
#define TC_OPEN_CKT_DETECT_CURRENT__1MA   0x3 << 18
// tc - custom address
#define TC_CUSTOM_ADDRESS_LSB 6
// tc - custom length-1
#define TC_CUSTOM_LENGTH_1_LSB 0
// tc - custom values
#define TC_CUSTOM_VALUES_LSB 31

//**********************************************
// -- Off-Chip Diode --
//**********************************************
// diode - differential?
#define DIODE_DIFFERENTIAL_LSB 26
#define DIODE_DIFFERENTIAL 0x0 << 26
#define DIODE_SINGLE_ENDED 0x1 << 26
// diode - num readings
#define DIODE_NUM_READINGS_LSB 25
#define DIODE_NUM_READINGS__2 0x0 << 25
#define DIODE_NUM_READINGS__3 0x1 << 25
// diode - averaging on?
#define DIODE_AVERAGING_ON_LSB 24
#define DIODE_AVERAGING_OFF 0x0 << 24
#define DIODE_AVERAGING_ON  0x1 << 24
// diode - current
#define DIODE_CURRENT_LSB 22
#define DIODE_CURRENT__10UA_40UA_80UA   0x0 << 22
#define DIODE_CURRENT__20UA_80UA_160UA  0x1 << 22
#define DIODE_CURRENT__40UA_160UA_320UA 0x2 << 22
#define DIODE_CURRENT__80UA_320UA_640UA 0x3 << 22
// diode - ideality factor(eta)
#define DIODE_IDEALITY_FACTOR_LSB 0

//**********************************************
// -- GLOBAL CONFIGURATION CONSTANTS --
//**********************************************
#define REJECTION__50_60_HZ 0x00
#define REJECTION__60_HZ    0x01
#define REJECTION__50_HZ    0x02
#define TEMP_UNIT__C        0x00
#define TEMP_UNIT__F        0x04

//**********************************************
// -- STATUS BYTE CONSTANTS --
//**********************************************
#define SENSOR_HARD_FAILURE 0x80
#define ADC_HARD_FAILURE    0x40
#define CJ_HARD_FAILURE     0x20
#define CJ_SOFT_FAILURE     0x10
#define SENSOR_ABOVE        0x08
#define SENSOR_BELOW        0x04
#define ADC_RANGE_ERROR     0x02
#define VALID               0x01

//**********************************************
// -- ADDRESSES --
//**********************************************
#define VOUT_CH_BASE		0x060
#define READ_CH_BASE		0x010

//**********************************************
// -- EEPROM --
//**********************************************
#define EEPROM_START_ADDRESS	0x0B0
#define CONNECT_TO_EEPROM	0xA53C0F5A
#define WRITE_TO_EEPROM		0x95
#define READ_FROM_EEPROM	0x96

#define READ_MULTIPLE_CHANNELS	0x80
#define READ_START_ADDRESSS	0x000


#endif	// LTC2984_CFG_H
