
extern "C" __declspec(dllexport) __stdcall void setDevName(PCHAR i);

extern "C" __declspec(dllexport) __stdcall unsigned char openQHY5V(void);
extern "C" __declspec(dllexport) __stdcall void w_i2c(unsigned char addr,unsigned short dat);
extern "C" __declspec(dllexport) __stdcall void AGC_enable(int i);
extern "C" __declspec(dllexport) __stdcall void AEC_enable(int i);
extern "C" __declspec(dllexport) __stdcall void HighGainBoost(unsigned char i);
extern "C" __declspec(dllexport) __stdcall void setQHY5VGlobalGain(unsigned short i);
extern "C" __declspec(dllexport) __stdcall void bitCompanding(int i);

extern "C" __declspec(dllexport) __stdcall void ReadMode(int RowFlip,int ColumnFlip,int ShowDarkRows,int ShowDarkColumns);
extern "C" __declspec(dllexport) __stdcall void setTotalShutterWidth(unsigned short width);
extern "C" __declspec(dllexport) __stdcall void LongExpMode(int i);
extern "C" __declspec(dllexport) __stdcall void HighDynamic(int i);
extern "C" __declspec(dllexport) __stdcall void ADC_Vref(unsigned char i);
extern "C" __declspec(dllexport) __stdcall void HighDynamic_Voltage(unsigned char i,unsigned char j);

extern "C" __declspec(dllexport) __stdcall void setLongExpTime(unsigned long i);


extern "C" __declspec(dllexport) __stdcall void RowNoiseReductionMethod(unsigned char i);
extern "C" __declspec(dllexport) __stdcall void BlackCalibration(unsigned char i);       //0= not enable   1=enable
extern "C" __declspec(dllexport) __stdcall void BlackOffset(int i);
extern "C" __declspec(dllexport) __stdcall void RowNoiseConstant(unsigned char i);




extern "C" __declspec(dllexport) __stdcall void getFullSizeImage(unsigned char *img);
extern "C" __declspec(dllexport) __stdcall void QHY5VInit(void);
