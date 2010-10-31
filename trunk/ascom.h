extern bool ASCOM_ConnectScope(wxString& scope_ID);  // returns true if failed to connect or canceled
extern bool ASCOM_OpenChooser(wxString& scope_ID);
extern void ASCOM_PulseGuideScope(int direction, int duration);
extern bool ASCOM_IsMoving(); // true = slewing or guiding, false = stable
extern void ASCOM_NudgeScope(int direction);


