#pragma once

#include "yrpp/platform/ABI.h"
#include <windows.h>
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/Interfaces.h"

namespace WOLAPI {
    // Forward references and typedefs
    interface IRTPatcher;
    interface IRTPatcherEvent;
    interface IChat;
    interface IChatEvent;
    /*disp*/interface IDownload;
    interface IDownloadEvent;
    interface INetUtil;
    interface INetUtilEvent;
    interface IChat2;
    interface IChat2Event;

    enum ELocale {
        LOC_UNKNOWN = 0,
        LOC_OTHER = 1,
        LOC_USA = 2,
        LOC_CANADA = 3,
        LOC_UK = 4,
        LOC_GERMANY = 5,
        LOC_FRANCE = 6,
        LOC_SPAIN = 7,
        LOC_NETHERLANDS = 8,
        LOC_BELGIUM = 9,
        LOC_AUSTRIA = 10,
        LOC_SWITZERLAND = 11,
        LOC_ITALY = 12,
        LOC_DENMARK = 13,
        LOC_SWEDEN = 14,
        LOC_NORWAY = 15,
        LOC_FINLAND = 16,
        LOC_ISRAEL = 17,
        LOC_SOUTH_AFRICA = 18,
        LOC_JAPAN = 19,
        LOC_SOUTH_KOREA = 20,
        LOC_CHINA = 21,
        LOC_SINGAPORE = 22,
        LOC_TAIWAN = 23,
        LOC_MALAYSIA = 24,
        LOC_AUSTRALIA = 25,
        LOC_NEW_ZEALAND = 26,
        LOC_BRAZIL = 27,
        LOC_THAILAND = 28,
        LOC_ARGENTINA = 29,
        LOC_PHILIPPINES = 30,
        LOC_GREECE = 31,
        LOC_IRELAND = 32,
        LOC_POLAND = 33,
        LOC_PORTUGAL = 34,
        LOC_MEXICO = 35,
        LOC_RUSSIA = 36,
        LOC_TURKEY = 37
    };

    struct TServer {
        int gametype;
        int chattype;
        int timezone;
        float longitude;
        float lattitude;
        TServer* next;
        char name[71];
        char connlabel[5];
        char conndata[128];
        char login[10];
        char password[10];
    };

    struct TChannel {
        int type;
        unsigned int minUsers;
        unsigned int maxUsers;
        unsigned int currentUsers;
        unsigned int official;
        unsigned int tournament;
        unsigned int ingame;
        unsigned int flags;
        unsigned long reserved;
        unsigned long ipaddr;
        int latency;
        int hidden;
        TChannel* next;
        char name[17];
        char topic[81];
        char location[65];
        char key[9];
        char exInfo[41];
    };

    struct TUser {
        unsigned int flags;
        int group;
        unsigned long reserved;
        unsigned long reserved2;
        unsigned long reserved3;
        unsigned long squadID;
        unsigned long ipaddr;
        unsigned long squad_icon;
        TUser* next;
        char name[10];
        char squadname[41];
        char squadabbrev[10];
        ELocale Locale;
        int team;
    };

    struct TUpdate {
        unsigned long SKU;
        unsigned long version;
        int required;
        TUpdate* next;
        char Server[65];
        char patchpath[256];
        char patchfile[33];
        char login[33];
        char password[65];
        char localpath[256];
    };

    struct TSquad {
        unsigned long id;
        int SKU;
        int members;
        int color1;
        int color2;
        int color3;
        int icon1;
        int icon2;
        int icon3;
        TSquad* next;
        int rank;
        int team;
        int status;
        char email[81];
        char icq[17];
        char motto[81];
        char URL[129];
        char name[41];
        char abbreviation[41];
    };

    struct TLadder {
        unsigned int SKU;
        unsigned int team_no;
        unsigned int wins;
        unsigned int losses;
        unsigned int points;
        unsigned int kills;
        unsigned int rank;
        unsigned int rung;
        unsigned int disconnects;
        unsigned int team_rung;
        unsigned int provisional;
        unsigned int last_game_date;
        unsigned int win_streak;
        unsigned int reserved1;
        unsigned int reserved2;
        TLadder* next;
        char login_name[40];
        ELocale Locale;
    };

    struct THighscore {
        unsigned int SKU;
        unsigned int wins;
        unsigned int losses;
        unsigned int points;
        unsigned int rank;
        unsigned int accomplishments;
        THighscore* next;
        char login_name[40];
    };

    enum GTYPE_ {
        Server_ = 0,
        Channel_ = 1,
        CLIENT_ = 2
    };

    // {925CDEDE-71B9-11D1-B1C5-006097176556}
    interface IRTPatcher : IUnknown
    {
        virtual HRESULT YRPP_STDCALL ApplyPatch( LPSTR destpath, LPSTR filename);
        virtual HRESULT YRPP_STDCALL PumpMessages();
    };

    // {925CDEE3-71B9-11D1-B1C5-006097176556}
    interface IRTPatcherEvent : IUnknown
    {
        virtual HRESULT YRPP_STDCALL OnProgress(LPSTR filename, int progress);
        virtual HRESULT YRPP_STDCALL OnTermination(long success);
    };

    // {4DD3BAF4-7579-11D1-B1C6-006097176556}
    interface IChat : IUnknown
    {
        virtual HRESULT YRPP_STDCALL PumpMessages();
        virtual HRESULT YRPP_STDCALL RequestServerList(
                unsigned long SKU,
                unsigned long current_version,
                LPSTR loginname,
                LPSTR password,
                int timeout);
        virtual HRESULT YRPP_STDCALL RequestConnection(TServer* Server, int timeout, int domangle);
        virtual HRESULT YRPP_STDCALL RequestChannelList(int channelType, int autoping);
        virtual HRESULT YRPP_STDCALL RequestChannelCreate(TChannel* Channel);
        virtual HRESULT YRPP_STDCALL RequestChannelJoin(TChannel* Channel);
        virtual HRESULT YRPP_STDCALL RequestChannelLeave();
        virtual HRESULT YRPP_STDCALL RequestUserList();
        virtual HRESULT YRPP_STDCALL RequestPublicMessage(LPSTR message);
        virtual HRESULT YRPP_STDCALL RequestPrivateMessage(TUser* users, LPSTR message);
        virtual HRESULT YRPP_STDCALL RequestLogout();
        virtual HRESULT YRPP_STDCALL RequestPrivateGameOptions(TUser* users, LPSTR options);
        virtual HRESULT YRPP_STDCALL RequestPublicGameOptions(LPSTR options);
        virtual HRESULT YRPP_STDCALL RequestPublicAction(LPSTR action);
        virtual HRESULT YRPP_STDCALL RequestPrivateAction(TUser* users, LPSTR action);
        virtual HRESULT YRPP_STDCALL RequestGameStart(TUser* users);
        virtual HRESULT YRPP_STDCALL RequestChannelTopic(LPSTR topic);
        virtual HRESULT YRPP_STDCALL GetVersion(unsigned long* version);
        virtual HRESULT YRPP_STDCALL RequestUserKick(TUser* User);
        virtual HRESULT YRPP_STDCALL RequestUserIP(TUser* User);
        virtual HRESULT YRPP_STDCALL GetGametypeInfo(
                unsigned int gtype,
                int icon_size,
                unsigned char** bitmap,
                int* bmp_bytes,
                LPSTR* name,
                LPSTR* URL);
        virtual HRESULT YRPP_STDCALL RequestFind(TUser* User);
        virtual HRESULT YRPP_STDCALL RequestPage(TUser* User, LPSTR message);
        virtual HRESULT YRPP_STDCALL SetFindPage(int findOn, int pageOn);
        virtual HRESULT YRPP_STDCALL SetSquelch(TUser* User, int squelch);
        virtual HRESULT YRPP_STDCALL GetSquelch(TUser* User);
        virtual HRESULT YRPP_STDCALL SetChannelFilter(int channelType);
        virtual HRESULT YRPP_STDCALL RequestGameEnd();
        virtual HRESULT YRPP_STDCALL SetLangFilter(int onoff);
        virtual HRESULT YRPP_STDCALL RequestChannelBan(LPSTR name, int ban);
        virtual HRESULT YRPP_STDCALL GetGametypeList(LPSTR* list);
        virtual HRESULT YRPP_STDCALL GetHelpURL(LPSTR* URL);
        virtual HRESULT YRPP_STDCALL SetProductSKU(unsigned long SKU);
        virtual HRESULT YRPP_STDCALL GetNick(int num, LPSTR* nick, LPSTR* pass);
        virtual HRESULT YRPP_STDCALL SetNick(int num, LPSTR nick, LPSTR pass, int domangle);
        virtual HRESULT YRPP_STDCALL GetLobbyCount(int* count);
        virtual HRESULT YRPP_STDCALL RequestRawMessage(LPSTR ircmsg);
        virtual HRESULT YRPP_STDCALL GetAttributeValue(LPSTR attrib, LPSTR* value);
        virtual HRESULT YRPP_STDCALL SetAttributeValue(LPSTR attrib, LPSTR value);
        virtual HRESULT YRPP_STDCALL SetChannelExInfo(LPSTR info);
        virtual HRESULT YRPP_STDCALL StopAutoping();
        virtual HRESULT YRPP_STDCALL RequestSquadInfo(unsigned long id);
        virtual HRESULT YRPP_STDCALL RequestSetTeam(int team);
        virtual HRESULT YRPP_STDCALL RequestSetLocale(ELocale Locale);
        virtual HRESULT YRPP_STDCALL RequestUserLocale(TUser* users);
        virtual HRESULT YRPP_STDCALL RequestUserTeam(TUser* users);
        virtual HRESULT YRPP_STDCALL GetNickLocale(int nicknum, ELocale* Locale);
        virtual HRESULT YRPP_STDCALL SetNickLocale(int nicknum, ELocale Locale);
        virtual HRESULT YRPP_STDCALL GetLocaleString(LPSTR* loc_string, ELocale Locale);
        virtual HRESULT YRPP_STDCALL GetLocaleCount(int* num);
        virtual HRESULT YRPP_STDCALL SetClientVersion(unsigned long version);
        virtual HRESULT YRPP_STDCALL SetCodepageFilter(int filter);
        virtual HRESULT YRPP_STDCALL RequestBuddyList();
        virtual HRESULT YRPP_STDCALL RequestBuddyAdd(TUser* newbuddy);
        virtual HRESULT YRPP_STDCALL RequestBuddyDelete(TUser* buddy);
        virtual HRESULT YRPP_STDCALL RequestPublicUnicodeMessage(unsigned short* message);
        virtual HRESULT YRPP_STDCALL RequestPrivateUnicodeMessage(TUser* users, unsigned short* message);
        virtual HRESULT YRPP_STDCALL RequestPublicUnicodeAction(unsigned short* action);
        virtual HRESULT YRPP_STDCALL RequestPrivateUnicodeAction(TUser* users, unsigned short* action);
        virtual HRESULT YRPP_STDCALL RequestUnicodePage(TUser* User, unsigned short* message);
        virtual HRESULT YRPP_STDCALL RequestSetPlayerCount(unsigned int currentPlayers, unsigned int maxPlayers);
        virtual HRESULT YRPP_STDCALL RequestServerTime();
        virtual HRESULT YRPP_STDCALL RequestInsiderStatus(TUser* users);
        virtual HRESULT YRPP_STDCALL RequestSetLocalIP();
    };

    // {4DD3BAF6-7579-11D1-B1C6-006097176556}
    interface IChatEvent : IUnknown
    {
        virtual HRESULT YRPP_STDCALL OnServerList(HRESULT res, TServer* servers);
        virtual HRESULT YRPP_STDCALL OnUpdateList(HRESULT res, TUpdate* updates);
        virtual HRESULT YRPP_STDCALL OnServerError(HRESULT res, LPSTR ircmsg);
        virtual HRESULT YRPP_STDCALL OnConnection(HRESULT res, LPSTR motd);
        virtual HRESULT YRPP_STDCALL OnMessageOfTheDay(HRESULT res, LPSTR motd);
        virtual HRESULT YRPP_STDCALL OnChannelList(HRESULT res, TChannel* channels);
        virtual HRESULT YRPP_STDCALL OnChannelCreate(HRESULT res, TChannel* Channel);
        virtual HRESULT YRPP_STDCALL OnChannelJoin(HRESULT res, TChannel* Channel, TUser* User);
        virtual HRESULT YRPP_STDCALL OnChannelLeave(HRESULT res, TChannel* Channel, TUser* User);
        virtual HRESULT YRPP_STDCALL OnChannelTopic(HRESULT res, TChannel* Channel, LPSTR topic);
        virtual HRESULT YRPP_STDCALL OnPrivateAction(HRESULT res, TUser* User, LPSTR action);
        virtual HRESULT YRPP_STDCALL OnPublicAction(HRESULT res, TChannel* Channel, TUser* User, LPSTR action);
        virtual HRESULT YRPP_STDCALL OnUserList(HRESULT res, TChannel* Channel, TUser* users);
        virtual HRESULT YRPP_STDCALL OnPublicMessage(HRESULT res, TChannel* Channel, TUser* User, LPSTR message);
        virtual HRESULT YRPP_STDCALL OnPrivateMessage(HRESULT res, TUser* User, LPSTR message);
        virtual HRESULT YRPP_STDCALL OnSystemMessage(HRESULT res, LPSTR message);
        virtual HRESULT YRPP_STDCALL OnNetStatus(HRESULT res);
        virtual HRESULT YRPP_STDCALL OnLogout(HRESULT status, TUser* User);
        virtual HRESULT YRPP_STDCALL OnPrivateGameOptions(HRESULT res, TUser* User, LPSTR options);
        virtual HRESULT YRPP_STDCALL OnPublicGameOptions(HRESULT res, TChannel* Channel, TUser* User, LPSTR options);
        virtual HRESULT YRPP_STDCALL OnGameStart(HRESULT res, TChannel* Channel, TUser* users, int gameid);
        virtual HRESULT YRPP_STDCALL OnUserKick(HRESULT res, TChannel* Channel, TUser* kicked, TUser* kicker);
        virtual HRESULT YRPP_STDCALL OnUserIP(HRESULT res, TUser* User);
        virtual HRESULT YRPP_STDCALL OnFind(HRESULT res, TChannel* chan);
        virtual HRESULT YRPP_STDCALL OnPageSend(HRESULT res);
        virtual HRESULT YRPP_STDCALL OnPaged(HRESULT res, TUser* User, LPSTR message);
        virtual HRESULT YRPP_STDCALL OnServerBannedYou(HRESULT res, long bannedTill);
        virtual HRESULT YRPP_STDCALL OnUserFlags(HRESULT res, LPSTR name, unsigned int flags, unsigned int mask);
        virtual HRESULT YRPP_STDCALL OnChannelBan(HRESULT res, LPSTR name, int banned);
        virtual HRESULT YRPP_STDCALL OnSquadInfo(HRESULT res, unsigned long id, TSquad* Squad);
        virtual HRESULT YRPP_STDCALL OnUserLocale(HRESULT res, TUser* users);
        virtual HRESULT YRPP_STDCALL OnUserTeam(HRESULT res, TUser* users);
        virtual HRESULT YRPP_STDCALL OnSetLocale(HRESULT res, ELocale newlocale);
        virtual HRESULT YRPP_STDCALL OnSetTeam(HRESULT res, int newteam);
        virtual HRESULT YRPP_STDCALL OnBuddyList(HRESULT res, TUser* buddy_list);
        virtual HRESULT YRPP_STDCALL OnBuddyAdd(HRESULT res, TUser* buddy_added);
        virtual HRESULT YRPP_STDCALL OnBuddyDelete(HRESULT res, TUser* buddy_deleted);
        virtual HRESULT YRPP_STDCALL OnPublicUnicodeMessage(HRESULT res, TChannel* Channel, TUser* User, unsigned short* message);
        virtual HRESULT YRPP_STDCALL OnPrivateUnicodeMessage(HRESULT res, TUser* User, unsigned short* message);
        virtual HRESULT YRPP_STDCALL OnPrivateUnicodeAction(HRESULT res, TUser* User, unsigned short* action);
        virtual HRESULT YRPP_STDCALL OnPublicUnicodeAction(HRESULT res, TChannel* Channel, TUser* User, unsigned short* action);
        virtual HRESULT YRPP_STDCALL OnPagedUnicode(HRESULT res, TUser* User, unsigned short* message);
        virtual HRESULT YRPP_STDCALL OnServerTime(HRESULT res, long stime);
        virtual HRESULT YRPP_STDCALL OnInsiderStatus(HRESULT res, TUser* users);
        virtual HRESULT YRPP_STDCALL OnSetLocalIP(HRESULT res, LPSTR message);
    };

    // {4DD3BAF5-7579-11D1-B1C6-006097176556}
    class Chat : public IChat, IChatEvent {
    };

    // {0BF5FCEB-9F03-11D1-9DC7-006097C54321}
    /**disp*/interface IDownload
    {
        void QueryInterface(
                GUID* riid,
                __out void** ppvObj);
        unsigned long AddRef();
        unsigned long Release();
        void DownloadFile(
                LPSTR Server,
                LPSTR login,
                LPSTR password,
                LPSTR file,
                LPSTR localfile,
                LPSTR regkey);
        void Abort();
        void PumpMessages();
    };

    // {6869E99D-9FB4-11D1-9DC8-006097C54321}
    interface IDownloadEvent : IUnknown
    {
        virtual HRESULT YRPP_STDCALL OnEnd();
        virtual HRESULT YRPP_STDCALL OnError(int error);
        virtual HRESULT YRPP_STDCALL OnProgressUpdate(
                int bytesread,
                int totalsize,
                int timetaken,
                int timeleft);
        virtual HRESULT YRPP_STDCALL OnQueryResume();
        virtual HRESULT YRPP_STDCALL OnStatusUpdate(int status);
    };

    class Download : public IDownload, IDownloadEvent {
    };

    // {B832B0AA-A7D3-11D1-97C3-00609706FA0C}
    interface INetUtil : IUnknown
    {
        virtual HRESULT YRPP_STDCALL RequestGameresSend(
                LPSTR host,
                int port,
                unsigned char* data,
                int length);
        virtual HRESULT YRPP_STDCALL RequestLadderSearch(
                LPSTR host,
                int port,
                LPSTR key,
                unsigned long SKU,
                int team,
                int cond,
                int sort,
                int number,
                int leading);
        virtual HRESULT YRPP_STDCALL RequestLadderList(
                LPSTR host,
                int port,
                LPSTR keys,
                unsigned long SKU,
                int team,
                int cond,
                int sort);
        virtual HRESULT YRPP_STDCALL RequestPing(
                LPSTR host,
                int timeout,
                int* handle);
        virtual HRESULT YRPP_STDCALL PumpMessages();
        virtual HRESULT YRPP_STDCALL GetAvgPing(
                unsigned long ip,
                int* avg);
        virtual HRESULT YRPP_STDCALL RequestNewNick(
                LPSTR nick,
                LPSTR pass,
                LPSTR email,
                LPSTR parentEmail,
                int newsletter,
                int shareinfo);
        virtual HRESULT YRPP_STDCALL RequestAgeCheck(
                int month,
                int day,
                int year,
                LPSTR email);
        virtual HRESULT YRPP_STDCALL RequestWDTState(
                LPSTR host,
                int port,
                unsigned char request);
        virtual HRESULT YRPP_STDCALL RequestLocaleLadderList(
                LPSTR host,
                int port,
                LPSTR keys,
                unsigned long SKU,
                int team,
                int cond,
                int sort,
                ELocale Locale);
        virtual HRESULT YRPP_STDCALL RequestLocaleLadderSearch(
                LPSTR host,
                int port,
                LPSTR key,
                unsigned long SKU,
                int team,
                int cond,
                int sort,
                int number,
                int leading,
                ELocale Locale);
        virtual HRESULT YRPP_STDCALL RequestHighscore(
                LPSTR host,
                int port,
                LPSTR keys,
                unsigned long SKU);
    };

    // {B832B0AC-A7D3-11D1-97C3-00609706FA0C}
    interface INetUtilEvent : IUnknown
    {
        virtual HRESULT YRPP_STDCALL OnPing(
                HRESULT res,
                int time,
                unsigned long ip,
                int handle);
        virtual HRESULT YRPP_STDCALL OnLadderList(
                HRESULT res,
                TLadder* list,
                int totalCount,
                long timeStamp,
                int keyRung);
        virtual HRESULT YRPP_STDCALL OnGameresSent(HRESULT res);
        virtual HRESULT YRPP_STDCALL OnNewNick(
                HRESULT res,
                LPSTR message,
                LPSTR nick,
                LPSTR pass);
        virtual HRESULT YRPP_STDCALL OnAgeCheck(
                HRESULT res,
                int years,
                int consent);
        virtual HRESULT YRPP_STDCALL OnWDTState(
                HRESULT res,
                unsigned char* state,
                int length);
        virtual HRESULT YRPP_STDCALL OnHighscore(
                HRESULT res,
                THighscore* list,
                int totalCount,
                long timeStamp,
                int keyRung);
    };

    // {B832B0AB-A7D3-11D1-97C3-00609706FA0C}
    class NetUtil : public INetUtil, INetUtilEvent {
    };

    // {8B938190-EF3F-11D1-9808-00609706FA0C}
    interface IChat2 : IUnknown
    {
        virtual HRESULT YRPP_STDCALL PumpMessages();
        virtual HRESULT YRPP_STDCALL RequestConnection(
                TServer* Server,
                int timeout);
        virtual HRESULT YRPP_STDCALL RequestMessage(
                unsigned long who,
                LPSTR message);
        virtual HRESULT YRPP_STDCALL GetTypeFromGID(
                unsigned long id,
                GTYPE_* type);
        virtual HRESULT YRPP_STDCALL RequestChannelList();
        virtual HRESULT YRPP_STDCALL RequestChannelJoin(LPSTR name);
        virtual HRESULT YRPP_STDCALL RequestChannelLeave(TChannel* chan);
        virtual HRESULT YRPP_STDCALL RequestUserList(TChannel* chan);
        virtual HRESULT YRPP_STDCALL RequestLogout();
        virtual HRESULT YRPP_STDCALL RequestChannelCreate(TChannel* chan);
        virtual HRESULT YRPP_STDCALL RequestRawCmd(LPSTR cmd);
    };

    // {8B938192-EF3F-11D1-9808-00609706FA0C}
    interface IChat2Event : IUnknown
    {
        virtual HRESULT YRPP_STDCALL OnNetStatus(HRESULT res);
        virtual HRESULT YRPP_STDCALL OnMessage(
                HRESULT res,
                TUser* User,
                LPSTR message);
        virtual HRESULT YRPP_STDCALL OnChannelList(
                HRESULT res,
                TChannel* list);
        virtual HRESULT YRPP_STDCALL OnChannelJoin(
                HRESULT res,
                TChannel* chan,
                TUser* User);
        virtual HRESULT YRPP_STDCALL OnLogin(HRESULT res);
        virtual HRESULT YRPP_STDCALL OnUserList(
                HRESULT res,
                TChannel* chan,
                TUser* users);
        virtual HRESULT YRPP_STDCALL OnChannelLeave(
                HRESULT res,
                TChannel* chan,
                TUser* User);
        virtual HRESULT YRPP_STDCALL OnChannelCreate(
                HRESULT res,
                TChannel* chan);
        virtual HRESULT YRPP_STDCALL OnUnknownLine(
                HRESULT res,
                LPSTR line);
    };

    // {8B938191-EF3F-11D1-9808-00609706FA0C}
    class Chat2 : public IChat2, IChat2Event {
    };

}
