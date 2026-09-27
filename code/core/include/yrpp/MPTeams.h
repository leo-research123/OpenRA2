#pragma once

// these classes handle alliances between players, eg Team:A B C D in the frontend

class MPTeam
{
public:
    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~MPTeam() RX;
    /// VA: unknown (legacy placeholder).
    virtual bool IsTeamIncluded(int idx) R0;
    /// VA: unknown (legacy placeholder).
    virtual bool SetPlayerTeam(int idxPlayer) R0;

    /// VA: 0x005D8D10.
    void AddToList(HWND hWnd)
        { JMP_THIS(0x5D8D10); }

protected:
    // Constructor
    /// VA: 0x005D8C50.
    MPTeam(wchar_t **title, int idx)
        { JMP_THIS(0x5D8C50); }

    MPTeam(noinit_t)
    { }

    // Properties

public:

    wchar_t* Title;
    int Index;
};

class MPCombatTeam : public MPTeam
{
public:
    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~MPCombatTeam() RX;

protected:
    // Constructor
    MPCombatTeam()
        : MPTeam(noinit_t())
    { }
};

class MPSiegeDefenderTeam : public MPTeam
{
public:
    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~MPSiegeDefenderTeam() RX;

protected:
    // Constructor
    /// VA: 0x005CAE10.
    MPSiegeDefenderTeam()
        : MPTeam(noinit_t())
    { JMP_THIS(0x5CAE10); }
};

class MPSiegeAttackerTeam : public MPTeam
{
public:
    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~MPSiegeAttackerTeam() RX

    // Constructor
    /// VA: 0x005CAEB0.
    MPSiegeAttackerTeam()
        : MPTeam(noinit_t())
    { JMP_THIS(0x5CAEB0); }
};

class MPObserverTeam : public MPTeam
{
public:
    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~MPObserverTeam() RX

    // Constructor
    /// VA: 0x005C9470.
    MPObserverTeam()
        : MPTeam(noinit_t())
    { JMP_THIS(0x5C9470); }
};
