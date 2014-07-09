
#ifdef  GUIDE_INDI

class ScopeINDI : public Scope
{
public:
    ScopeINDI(void) {
        m_Name = wxString("INDI");
    }

    virtual bool Connect(void);

    virtual bool Disconnect(void);

    virtual MOVE_RESULT Guide(GUIDE_DIRECTION direction, int duration);

};

#endif /* GUIDE_INDI */
