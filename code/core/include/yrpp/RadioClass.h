/*
    Base class for WHAT?? I DUNNO =(
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/MissionClass.h"

// forward declarations
class TechnoClass;

class NOVTABLE RadioClass : public MissionClass
{
public:
    /// VA: 0x65A820
    RadioCommand ReceiveCommand(TechnoClass* sender,RadioCommand command,AbstractClass*& data) override;
    /// VA: 0x0065AA80
    bool Limbo() override;
    /// VA: 0x0065AAC0
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x65AAC0); }
#else
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif
    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual ~RadioClass() RX;
#else
    virtual ~RadioClass();
#endif

    // RadioClass

    // these are oogly, westwood themselves admitted it, so it's probably even more of a wtf than the rest
    /// VA: 0x0065ACB0
#if defined(RA2_YRPP_GAME)
    virtual RadioCommand SendToFirstLink(RadioCommand command) { JMP_THIS(0x65ACB0); }
#else
    virtual RadioCommand SendToFirstLink(RadioCommand command);
#endif
    /// VA: 0x0065AAA0
    virtual RadioCommand SendCommand(RadioCommand command, TechnoClass* pRecipient);
    /// VA: 0x0065A970
    virtual RadioCommand SendCommandWithData(RadioCommand command, AbstractClass* &pInOut, TechnoClass* pRecipient);
    /// VA: 0x0065ACE0
    virtual void SendToEachLink(RadioCommand command);

    // get specific link
    TechnoClass* const& GetNthLink(int idx = 0) const {
        return this->RadioLinks[idx];
    }

    // whether any link is pLink
    /// VA: 0x0065AD50.
    bool ContainsLink(TechnoClass const* pLink) const;

    // note: null pointers will always return -1
    /// VA: 0x0065AD90.
    int FindLinkIndex(TechnoClass const* pLink) const;

    // iow: not full
    /// VA: 0x0065ADC0.
    bool HasFreeLink() const;

    // iow: not full; consider pIgnore's link empty
    /// VA: 0x0065ADF0.
    bool HasFreeLink(TechnoClass const* pIgnore) const;

    // iow. at least one link used
    /// VA: 0x0065AE30.
    bool HasAnyLink() const
        { for(int i=0;i<RadioLinks.Capacity;++i)if(RadioLinks[i])return true;return false; }

    // resizes the vector and nulls the new elements
    /// VA: 0x0065AE60.
    void SetLinkCount(int count);

    // Constructor
    /// VA: 0x0065A750.
#if defined(RA2_YRPP_GAME)
    RadioClass() noexcept
        : RadioClass(noinit_t())
    { JMP_THIS(0x65A750); }
#else
    RadioClass() noexcept;
#endif

protected:
    explicit __forceinline RadioClass(noinit_t) noexcept
        : MissionClass(noinit_t())
    { }

    // Properties

public:

    RadioCommand LastCommands[3]; // new command updates these
    DECLARE_PROPERTY(VectorClass<TechnoClass*>, RadioLinks);	//Docked units etc
};
