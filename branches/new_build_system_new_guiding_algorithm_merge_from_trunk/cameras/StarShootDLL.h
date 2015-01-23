typedef bool (CALLBACK* B_V_DLLFUNC)(void);
typedef bool (CALLBACK* B_I_DLLFUNC)(int);
typedef bool (CALLBACK* B_B_DLLFUNC)(bool);
typedef unsigned char* (CALLBACK* UCP_V_DLLFUNC)(void);
typedef unsigned short* (CALLBACK* USP_V_DLLFUNC)(void);
typedef unsigned int (CALLBACK* UI_V_DLLFUNC)(void);
typedef void (CALLBACK* V_V_DLLFUNC)(void);
typedef void (CALLBACK* V_UCp_DLLFUNC)(unsigned char*);
typedef unsigned char (CALLBACK* OCPREGFUNC)(unsigned long,unsigned char,unsigned char,unsigned char,
                                                            bool,bool,bool,bool,bool,bool,bool,bool,bool,bool);


