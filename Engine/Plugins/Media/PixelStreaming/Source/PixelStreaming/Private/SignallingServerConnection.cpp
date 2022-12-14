// Copyright Epic Games, Inc. All Rights Reserved.

#include "SignallingServerConnection.h"
#include "ToStringExtensions.h"

#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Misc/AssertionMacros.h"
#include "Logging/LogMacros.h"
#include "Settings.h"
#include "TimerManager.h"
#include "PixelStreamingDelegates.h"
#include "PlayerSession.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPixelStreamingSS, Log, VeryVerbose);
DEFINE_LOG_CATEGORY(LogPixelStreamingSS);

// This handles errors in Signalling Server (SS) messaging by logging them and disconnecting from SS
#define HANDLE_SS_ERROR(ErrorMsg, ...)                               \
	do                                                               \
	{                                                                \
		UE_LOG(LogPixelStreamingSS, Error, ErrorMsg, ##__VA_ARGS__); \
		WS->Close(4000, FString::Printf(ErrorMsg, ##__VA_ARGS__));   \
		return;                                                      \
	}                                                                \
	while (false);

#define HANDLE_PLAYER_SS_ERROR(PlayerId, ErrorMsg, ...)                                             \
	do                                                                                              \
	{                                                                                               \
		UE_LOG(LogPixelStreamingSS, Error, TEXT("player %s: ") ErrorMsg, *PlayerId, ##__VA_ARGS__); \
		SendDisconnectPlayer(PlayerId, FString::Printf(ErrorMsg, ##__VA_ARGS__));                   \
		return;                                                                                     \
	}                                                                                               \
	while (false);

UE::PixelStreaming::FSignallingServerConnection::FSignallingServerConnection(UE::PixelStreaming::FSignallingServerConnectionObserver& InObserver, const FString& InStreamerId)
	: Observer(InObserver), StreamerId(InStreamerId)
{
}

void UE::PixelStreaming::FSignallingServerConnection::Connect(const FString& Url)
{
	// Already have a websocket connection, no need to make another one
	if (WS)
	{
		return;
	}

	WS = FWebSocketsModule::Get().CreateWebSocket(Url, TEXT(""));

	OnConnectedHandle = WS->OnConnected().AddLambda([this]() { OnConnected(); });
	OnConnectionErrorHandle = WS->OnConnectionError().AddLambda([this](const FString& Error) { OnConnectionError(Error); });
	OnClosedHandle = WS->OnClosed().AddLambda([this](int32 StatusCode, const FString& Reason, bool bWasClean) { OnClosed(StatusCode, Reason, bWasClean); });
	OnMessageHandle = WS->OnMessage().AddLambda([this](const FString& Msg) { OnMessage(Msg); });

	UE_LOG(LogPixelStreamingSS, Log, TEXT("Connecting to SS %s"), *Url);
	WS->Connect();
}

void UE::PixelStreaming::FSignallingServerConnection::Disconnect()
{
	if (!WS)
	{
		return;
	}

	if (!IsEngineExitRequested())
	{
		GWorld->GetTimerManager().ClearTimer(TimerHandle_KeepAlive);
	}

	WS->OnConnected().Remove(OnConnectedHandle);
	WS->OnConnectionError().Remove(OnConnectionErrorHandle);
	WS->OnClosed().Remove(OnClosedHandle);
	WS->OnMessage().Remove(OnMessageHandle);

	WS->Close();
	WS = nullptr;
}

UE::PixelStreaming::FSignallingServerConnection::~FSignallingServerConnection()
{
	Disconnect();
}

void UE::PixelStreaming::FSignallingServerConnection::SendOffer(FPixelStreamingPlayerId PlayerId, const webrtc::SessionDescriptionInterface& SDP)
{
	FJsonObjectPtr OfferJson = MakeShared<FJsonObject>();
	OfferJson->SetStringField(TEXT("type"), TEXT("offer"));
	SetPlayerIdJson(OfferJson, PlayerId);

	std::string SdpAnsi;
	SDP.ToString(&SdpAnsi);
	FString SdpStr = ToString(SdpAnsi);
	OfferJson->SetStringField(TEXT("sdp"), SdpStr);

	UE_LOG(LogPixelStreamingSS, Verbose, TEXT("-> SS: offer\n%s"), *SdpStr);

	WS->Send(ToString(OfferJson, false));
}

void UE::PixelStreaming::FSignallingServerConnection::SendAnswer(FPixelStreamingPlayerId PlayerId, const webrtc::SessionDescriptionInterface& SDP)
{
	FJsonObjectPtr AnswerJson = MakeShared<FJsonObject>();
	AnswerJson->SetStringField(TEXT("type"), TEXT("answer"));
	SetPlayerIdJson(AnswerJson, PlayerId);

	std::string SdpAnsi;
	verifyf(SDP.ToString(&SdpAnsi), TEXT("Failed to serialise local SDP"));
	FString SdpStr = ToString(SdpAnsi);
	AnswerJson->SetStringField(TEXT("sdp"), SdpStr);

	UE_LOG(LogPixelStreamingSS, Verbose, TEXT("-> SS: answer\n%s"), *SdpStr);

	WS->Send(ToString(AnswerJson, false));
}

void UE::PixelStreaming::FSignallingServerConnection::SetPlayerIdJson(FJsonObjectPtr& JsonObject, FPixelStreamingPlayerId PlayerId)
{
	bool bSendAsInteger = UE::PixelStreaming::Settings::CVarSendPlayerIdAsInteger.GetValueOnAnyThread();
	if (bSendAsInteger)
	{
		int32 PlayerIdAsInt = PlayerIdToInt(PlayerId);
		JsonObject->SetNumberField(TEXT("playerId"), PlayerIdAsInt);
	}
	else
	{
		JsonObject->SetStringField(TEXT("playerId"), PlayerId);
	}
}

bool UE::PixelStreaming::FSignallingServerConnection::GetPlayerIdJson(const FJsonObjectPtr& Json, FPixelStreamingPlayerId& OutPlayerId)
{
	bool bSendAsInteger = UE::PixelStreaming::Settings::CVarSendPlayerIdAsInteger.GetValueOnAnyThread();
	if (bSendAsInteger)
	{
		uint32 PlayerIdInt;
		if (Json->TryGetNumberField(TEXT("playerId"), PlayerIdInt))
		{
			OutPlayerId = ToPlayerId(PlayerIdInt);
			return true;
		}
	}
	else if (Json->TryGetStringField(TEXT("playerId"), OutPlayerId))
	{
		return true;
	}

	UE_LOG(LogPixelStreamingSS, Error, TEXT("Failed to extracted player id offer json: %s"), *ToString(Json));
	return false;
}

void UE::PixelStreaming::FSignallingServerConnection::SendIceCandidate(const webrtc::IceCandidateInterface& IceCandidate)
{
	FJsonObjectPtr IceCandidateJson = MakeShared<FJsonObject>();

	IceCandidateJson->SetStringField(TEXT("type"), TEXT("iceCandidate"));

	FJsonObjectPtr CandidateJson = MakeShared<FJsonObject>();
	CandidateJson->SetStringField(TEXT("sdpMid"), IceCandidate.sdp_mid().c_str());
	CandidateJson->SetNumberField(TEXT("sdpMLineIndex"), static_cast<double>(IceCandidate.sdp_mline_index()));

	std::string CandidateAnsi;
	verifyf(IceCandidate.ToString(&CandidateAnsi), TEXT("Failed to serialize IceCandidate"));
	FString CandidateStr = ToString(CandidateAnsi);
	CandidateJson->SetStringField(TEXT("candidate"), CandidateStr);

	IceCandidateJson->SetObjectField(TEXT("candidate"), CandidateJson);

	UE_LOG(LogPixelStreamingSS, Verbose, TEXT("-> SS: ice-candidate\n%s"), *ToString(IceCandidateJson));

	WS->Send(ToString(IceCandidateJson, false));
}

void UE::PixelStreaming::FSignallingServerConnection::SendIceCandidate(FPixelStreamingPlayerId PlayerId, const webrtc::IceCandidateInterface& IceCandidate)
{
	FJsonObjectPtr IceCandidateJson = MakeShared<FJsonObject>();

	IceCandidateJson->SetStringField(TEXT("type"), TEXT("iceCandidate"));
	SetPlayerIdJson(IceCandidateJson, PlayerId);

	FJsonObjectPtr CandidateJson = MakeShared<FJsonObject>();
	CandidateJson->SetStringField(TEXT("sdpMid"), IceCandidate.sdp_mid().c_str());
	CandidateJson->SetNumberField(TEXT("sdpMLineIndex"), static_cast<double>(IceCandidate.sdp_mline_index()));

	std::string CandidateAnsi;
	verifyf(IceCandidate.ToString(&CandidateAnsi), TEXT("Failed to serialize IceCandidate"));
	FString CandidateStr = ToString(CandidateAnsi);
	CandidateJson->SetStringField(TEXT("candidate"), CandidateStr);

	IceCandidateJson->SetObjectField(TEXT("candidate"), CandidateJson);

	UE_LOG(LogPixelStreamingSS, Verbose, TEXT("-> SS: iceCandidate\n%s"), *ToString(IceCandidateJson));

	WS->Send(ToString(IceCandidateJson, false));
}

void UE::PixelStreaming::FSignallingServerConnection::KeepAlive()
{
	FJsonObjectPtr Json = MakeShared<FJsonObject>();
	double unixTime = FDateTime::UtcNow().ToUnixTimestamp();
	Json->SetStringField(TEXT("type"), TEXT("ping"));
	Json->SetNumberField(TEXT("time"), unixTime);
	FString Msg = ToString(Json, false);
	if (WS != nullptr && WS.IsValid() && WS->IsConnected())
	{
		WS->Send(Msg);
	}
}

void UE::PixelStreaming::FSignallingServerConnection::SendDisconnectPlayer(FPixelStreamingPlayerId PlayerId, const FString& Reason)
{
	FJsonObjectPtr Json = MakeShared<FJsonObject>();

	Json->SetStringField(TEXT("type"), TEXT("disconnectPlayer"));
	SetPlayerIdJson(Json, PlayerId);
	Json->SetStringField(TEXT("reason"), Reason);

	FString Msg = ToString(Json, false);
	UE_LOG(LogPixelStreamingSS, Verbose, TEXT("-> SS: iceCandidate\n%s"), *Msg);

	WS->Send(Msg);
}

void UE::PixelStreaming::FSignallingServerConnection::OnConnected()
{
	UE_LOG(LogPixelStreamingSS, Log, TEXT("Connected to SS"));

	//Send message to keep connection alive every 60 seconds
	GWorld->GetTimerManager().SetTimer(TimerHandle_KeepAlive, std::bind(&UE::PixelStreaming::FSignallingServerConnection::KeepAlive, this), KEEP_ALIVE_INTERVAL, true);

	if (UPixelStreamingDelegates* Delegates = UPixelStreamingDelegates::GetPixelStreamingDelegates())
	{
		Delegates->OnConnectedToSignallingServer.Broadcast();
	}
}

void UE::PixelStreaming::FSignallingServerConnection::OnConnectionError(const FString& Error)
{
	UE_LOG(LogPixelStreamingSS, Error, TEXT("Failed to connect to SS: %s"), *Error);
	Observer.OnSignallingServerDisconnected();
	GWorld->GetTimerManager().ClearTimer(TimerHandle_KeepAlive);

	if (UPixelStreamingDelegates* Delegates = UPixelStreamingDelegates::GetPixelStreamingDelegates())
	{
		Delegates->OnDisconnectedFromSignallingServer.Broadcast();
	}
}

void UE::PixelStreaming::FSignallingServerConnection::OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	UE_LOG(LogPixelStreamingSS, Log, TEXT("Connection to SS closed: \n\tstatus %d\n\treason: %s\n\twas clean: %s"), StatusCode, *Reason, bWasClean ? TEXT("true") : TEXT("false"));
	Observer.OnSignallingServerDisconnected();
	GWorld->GetTimerManager().ClearTimer(TimerHandle_KeepAlive);

	if (UPixelStreamingDelegates* Delegates = UPixelStreamingDelegates::GetPixelStreamingDelegates())
	{
		Delegates->OnDisconnectedFromSignallingServer.Broadcast();
	}
}

void UE::PixelStreaming::FSignallingServerConnection::OnMessage(const FString& Msg)
{
	UE_LOG(LogPixelStreamingSS, Log, TEXT("<- SS: %s"), *Msg);

	TSharedPtr<FJsonObject> JsonMsg;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Msg);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonMsg))
	{
		HANDLE_SS_ERROR(TEXT("Failed to parse SS message:\n%s"), *Msg);
	}

	FString MsgType;
	if (!JsonMsg->TryGetStringField(TEXT("type"), MsgType))
	{
		HANDLE_SS_ERROR(TEXT("Cannot find `type` field in SS message:\n%s"), *Msg);
	}

	if (MsgType == TEXT("identify"))
	{
		OnIdRequested();
	}
	else if (MsgType == TEXT("config"))
	{
		OnConfig(JsonMsg);
	}
	else if (MsgType == TEXT("offer") || MsgType == TEXT("answer"))
	{
		OnSessionDescription(JsonMsg);
	}
	else if (MsgType == TEXT("iceCandidate"))
	{
		if (JsonMsg->HasField(TEXT("playerId")))
		{
			OnPlayerIceCandidate(JsonMsg);
		}
		else
		{
			OnStreamerIceCandidate(JsonMsg);
		}
	}
	else if (MsgType == TEXT("playerCount"))
	{
		OnPlayerCount(JsonMsg);
	}
	else if (MsgType == TEXT("playerConnected"))
	{
		OnPlayerConnected(JsonMsg);
	}
	else if (MsgType == TEXT("playerDisconnected"))
	{
		OnPlayerDisconnected(JsonMsg);
	}
	else if (MsgType == TEXT("pong"))
	{
		//Do nothing, this is a keep alive message
	}
	else
	{
		UE_LOG(LogPixelStreamingSS, Error, TEXT("Unsupported message `%s` received from SS"), *MsgType);
		WS->Close(4001, TEXT("Unsupported message received: ") + MsgType);
	}
}

void UE::PixelStreaming::FSignallingServerConnection::OnIdRequested()
{
	FJsonObjectPtr Json = MakeShared<FJsonObject>();

	Json->SetStringField(TEXT("type"), TEXT("endpointId"));
	Json->SetStringField(TEXT("id"), StreamerId);

	FString Msg = ToString(Json, false);
	UE_LOG(LogPixelStreamingSS, Verbose, TEXT("-> SS: endpointId\n%s"), *Msg);

	WS->Send(Msg);
}

void UE::PixelStreaming::FSignallingServerConnection::OnConfig(const FJsonObjectPtr& Json)
{
	// SS sends `config` that looks like:
	// `{peerConnectionOptions: { 'iceServers': [{'urls': ['stun:34.250.222.95:19302', 'turn:34.250.222.95:19303']}] }}`
	// where `peerConnectionOptions` is `RTCConfiguration` (except in native `RTCConfiguration` "iceServers" = "servers").
	// As `RTCConfiguration` doesn't implement parsing from a string (or `ToString` method),
	// we just get `stun`/`turn` URLs from it and ignore other options

	const TSharedPtr<FJsonObject>* PeerConnectionOptions;
	if (!Json->TryGetObjectField(TEXT("peerConnectionOptions"), PeerConnectionOptions))
	{
		HANDLE_SS_ERROR(TEXT("Cannot find `peerConnectionOptions` field in SS config\n%s"), *ToString(Json));
	}

	webrtc::PeerConnectionInterface::RTCConfiguration RTCConfig;

	const TArray<TSharedPtr<FJsonValue>>* IceServers;
	if ((*PeerConnectionOptions)->TryGetArrayField(TEXT("iceServers"), IceServers))
	{
		for (const TSharedPtr<FJsonValue>& IceServerVal : *IceServers)
		{
			const TSharedPtr<FJsonObject>* IceServerJson;
			if (!IceServerVal->TryGetObject(IceServerJson))
			{
				HANDLE_SS_ERROR(TEXT("Failed to parse SS config: `iceServer` - not an object\n%s"), *ToString(*PeerConnectionOptions));
			}

			RTCConfig.servers.push_back(webrtc::PeerConnectionInterface::IceServer{});
			webrtc::PeerConnectionInterface::IceServer& IceServer = RTCConfig.servers.back();

			TArray<FString> Urls;
			if ((*IceServerJson)->TryGetStringArrayField(TEXT("urls"), Urls))
			{
				for (const FString& Url : Urls)
				{
					IceServer.urls.push_back(to_string(Url));
				}
			}
			else
			{
				// in the RTC Spec, "urls" can be an array or a single string
				// https://www.w3.org/TR/webrtc/#dictionary-rtciceserver-members
				FString UrlsSingle;
				if ((*IceServerJson)->TryGetStringField(TEXT("urls"), UrlsSingle))
				{
					IceServer.urls.push_back(to_string(UrlsSingle));
				}
			}

			FString Username;
			if ((*IceServerJson)->TryGetStringField(TEXT("username"), Username))
			{
				IceServer.username = to_string(Username);
			}

			FString Credential;
			if ((*IceServerJson)->TryGetStringField(TEXT("credential"), Credential))
			{
				IceServer.password = to_string(Credential);
			}
		}
	}

	// force `UnifiedPlan` as we control both ends of WebRTC streaming
	RTCConfig.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

	Observer.OnConfig(RTCConfig);
}

void UE::PixelStreaming::FSignallingServerConnection::OnSessionDescription(const FJsonObjectPtr& Json)
{
	webrtc::SdpType Type = Json->GetStringField(TEXT("type")) == TEXT("offer") ? webrtc::SdpType::kOffer : webrtc::SdpType::kAnswer;

	FString Sdp;
	if (!Json->TryGetStringField(TEXT("sdp"), Sdp))
	{
		HANDLE_SS_ERROR(TEXT("Cannot find `sdp` in Streamer's answer\n%s"), *ToString(Json));
	}

	FPixelStreamingPlayerId PlayerId;
	bool bGotPlayerId = GetPlayerIdJson(Json, PlayerId);
	if (!bGotPlayerId)
	{
		HANDLE_SS_ERROR(TEXT("Failed to get `playerId` from `offer/answer` message\n%s"), *ToString(Json));
	}

	Observer.OnSessionDescription(PlayerId, Type, Sdp);
}

void UE::PixelStreaming::FSignallingServerConnection::OnStreamerIceCandidate(const FJsonObjectPtr& Json)
{
	const FJsonObjectPtr* CandidateJson;
	if (!Json->TryGetObjectField(TEXT("candidate"), CandidateJson))
	{
		HANDLE_SS_ERROR(TEXT("Failed to get `candiate` from remote `iceCandidate` message\n%s"), *ToString(Json));
	}

	FString SdpMid;
	if (!(*CandidateJson)->TryGetStringField(TEXT("sdpMid"), SdpMid))
	{
		HANDLE_SS_ERROR(TEXT("Failed to get `sdpMid` from remote `iceCandidate` message\n%s"), *ToString(Json));
	}

	int32 SdpMLineIndex = 0;
	if (!(*CandidateJson)->TryGetNumberField(TEXT("sdpMlineIndex"), SdpMLineIndex))
	{
		HANDLE_SS_ERROR(TEXT("Failed to get `sdpMlineIndex` from remote `iceCandidate` message\n%s"), *ToString(Json));
	}

	FString CandidateStr;
	if (!(*CandidateJson)->TryGetStringField(TEXT("candidate"), CandidateStr))
	{
		HANDLE_SS_ERROR(TEXT("Failed to get `candidate` from remote `iceCandidate` message\n%s"), *ToString(Json));
	}

	webrtc::SdpParseError Error;
	std::unique_ptr<webrtc::IceCandidateInterface> Candidate(webrtc::CreateIceCandidate(to_string(SdpMid), SdpMLineIndex, to_string(CandidateStr), &Error));
	if (!Candidate)
	{
		HANDLE_SS_ERROR(TEXT("Failed to parse remote `iceCandidate` message\n%s"), *ToString(Json));
	}

	Observer.OnRemoteIceCandidate(TUniquePtr<webrtc::IceCandidateInterface>{ Candidate.release() });
}

void UE::PixelStreaming::FSignallingServerConnection::OnPlayerIceCandidate(const FJsonObjectPtr& Json)
{
	FPixelStreamingPlayerId PlayerId;
	bool bGotPlayerId = GetPlayerIdJson(Json, PlayerId);
	if (!bGotPlayerId)
	{
		HANDLE_PLAYER_SS_ERROR(PlayerId, TEXT("Failed to get `playerId` from remote `iceCandidate` message\n%s"), *ToString(Json));
	}

	const FJsonObjectPtr* CandidateJson;
	if (!Json->TryGetObjectField(TEXT("candidate"), CandidateJson))
	{
		HANDLE_PLAYER_SS_ERROR(PlayerId, TEXT("Failed to get `candiate` from remote `iceCandidate` message\n%s"), *ToString(Json));
	}

	FString SdpMid;
	if (!(*CandidateJson)->TryGetStringField(TEXT("sdpMid"), SdpMid))
	{
		HANDLE_PLAYER_SS_ERROR(PlayerId, TEXT("Failed to get `sdpMid` from remote `iceCandidate` message\n%s"), *ToString(Json));
	}

	int32 SdpMLineIndex = 0;
	if (!(*CandidateJson)->TryGetNumberField(TEXT("sdpMlineIndex"), SdpMLineIndex))
	{
		HANDLE_PLAYER_SS_ERROR(PlayerId, TEXT("Failed to get `sdpMlineIndex` from remote `iceCandidate` message\n%s"), *ToString(Json));
	}

	FString CandidateStr;
	if (!(*CandidateJson)->TryGetStringField(TEXT("candidate"), CandidateStr))
	{
		HANDLE_PLAYER_SS_ERROR(PlayerId, TEXT("Failed to get `candidate` from remote `iceCandidate` message\n%s"), *ToString(Json));
	}

	Observer.OnRemoteIceCandidate(PlayerId, to_string(SdpMid), SdpMLineIndex, to_string(CandidateStr));
}

void UE::PixelStreaming::FSignallingServerConnection::OnPlayerCount(const FJsonObjectPtr& Json)
{
	uint32 Count;
	if (!Json->TryGetNumberField(TEXT("count"), Count))
	{
		HANDLE_SS_ERROR(TEXT("Failed to get `count` from `playerCount` message\n%s"), *ToString(Json));
	}

	Observer.OnPlayerCount(Count);
}

void UE::PixelStreaming::FSignallingServerConnection::OnPlayerConnected(const FJsonObjectPtr& Json)
{
	FPixelStreamingPlayerId PlayerId;
	bool bGotPlayerId = GetPlayerIdJson(Json, PlayerId);
	if (!bGotPlayerId)
	{
		HANDLE_SS_ERROR(TEXT("Failed to get `playerId` from `join` message\n%s"), *ToString(Json));
	}
	int Flags = 0;

	// Default to always making datachannel, unless explicitly set to false.
	bool bMakeDataChannel = true;
	Json->TryGetBoolField(TEXT("datachannel"), bMakeDataChannel);

	// Default peer is not an SFU, unless explictly set as SFU
	bool bIsSFU = false;
	Json->TryGetBoolField(TEXT("sfu"), bIsSFU);

	Flags |= bMakeDataChannel ? UE::PixelStreaming::Protocol::EPlayerFlags::PSPFlag_SupportsDataChannel : 0;
	Flags |= bIsSFU ? UE::PixelStreaming::Protocol::EPlayerFlags::PSPFlag_IsSFU : 0;
	Observer.OnPlayerConnected(PlayerId, Flags);
}

void UE::PixelStreaming::FSignallingServerConnection::OnPlayerDisconnected(const FJsonObjectPtr& Json)
{
	FPixelStreamingPlayerId PlayerId;
	bool bGotPlayerId = GetPlayerIdJson(Json, PlayerId);
	if (!bGotPlayerId)
	{
		HANDLE_SS_ERROR(TEXT("Failed to get `playerId` from `playerDisconnected` message\n%s"), *ToString(Json));
	}

	Observer.OnPlayerDisconnected(PlayerId);
}

#undef HANDLE_PLAYER_SS_ERROR
#undef HANDLE_SS_ERROR