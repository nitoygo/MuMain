﻿// <copyright file="PacketFunctions.cpp" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

//------------------------------------------------------------------------------
// <auto-generated>
//     This source code was auto-generated by an XSL transformation.
//     Do not change this file. Instead, change the XML data which contains
//     the packet definitions and re-run the transformation (publish/rebuild the
//     managed library project).
// </auto-generated>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "PacketFunctions_ClientToServer.h"
#include "PacketBindings_ClientToServer.h"


void PacketFunctions_ClientToServer::SendPing(uint32_t tickCount, uint16_t attackSpeed)
{
    dotnet_SendPing(this->GetHandle(), tickCount, attackSpeed);
}

void PacketFunctions_ClientToServer::SendChecksumResponse(uint32_t checksum)
{
    dotnet_SendChecksumResponse(this->GetHandle(), checksum);
}

void PacketFunctions_ClientToServer::SendPublicChatMessage(const wchar_t* character, const wchar_t* message)
{
    dotnet_SendPublicChatMessage(this->GetHandle(), character, message);
}

void PacketFunctions_ClientToServer::SendWhisperMessage(const wchar_t* receiverName, const wchar_t* message)
{
    dotnet_SendWhisperMessage(this->GetHandle(), receiverName, message);
}

void PacketFunctions_ClientToServer::SendLoginLongPassword(const BYTE* username, uint32_t usernameByteLength, const BYTE* password, uint32_t passwordByteLength, uint32_t tickCount, const BYTE* clientVersion, uint32_t clientVersionByteLength, const BYTE* clientSerial, uint32_t clientSerialByteLength)
{
    dotnet_SendLoginLongPassword(this->GetHandle(), username, usernameByteLength, password, passwordByteLength, tickCount, clientVersion, clientVersionByteLength, clientSerial, clientSerialByteLength);
}

void PacketFunctions_ClientToServer::SendLoginShortPassword(const BYTE* username, uint32_t usernameByteLength, const BYTE* password, uint32_t passwordByteLength, uint32_t tickCount, const BYTE* clientVersion, uint32_t clientVersionByteLength, const BYTE* clientSerial, uint32_t clientSerialByteLength)
{
    dotnet_SendLoginShortPassword(this->GetHandle(), username, usernameByteLength, password, passwordByteLength, tickCount, clientVersion, clientVersionByteLength, clientSerial, clientSerialByteLength);
}

void PacketFunctions_ClientToServer::SendLogin075(const BYTE* username, uint32_t usernameByteLength, const BYTE* password, uint32_t passwordByteLength, uint32_t tickCount, const BYTE* clientVersion, uint32_t clientVersionByteLength, const BYTE* clientSerial, uint32_t clientSerialByteLength)
{
    dotnet_SendLogin075(this->GetHandle(), username, usernameByteLength, password, passwordByteLength, tickCount, clientVersion, clientVersionByteLength, clientSerial, clientSerialByteLength);
}

void PacketFunctions_ClientToServer::SendLogOut(uint32_t type)
{
    dotnet_SendLogOut(this->GetHandle(), type);
}

void PacketFunctions_ClientToServer::SendLogOutByCheatDetection(BYTE param, BYTE type)
{
    dotnet_SendLogOutByCheatDetection(this->GetHandle(), param, type);
}

void PacketFunctions_ClientToServer::SendResetCharacterPointRequest()
{
    dotnet_SendResetCharacterPointRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendPlayerShopSetItemPrice(BYTE itemSlot, uint32_t price)
{
    dotnet_SendPlayerShopSetItemPrice(this->GetHandle(), itemSlot, price);
}

void PacketFunctions_ClientToServer::SendPlayerShopOpen(const wchar_t* storeName)
{
    dotnet_SendPlayerShopOpen(this->GetHandle(), storeName);
}

void PacketFunctions_ClientToServer::SendPlayerShopClose()
{
    dotnet_SendPlayerShopClose(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendPlayerShopItemListRequest(uint16_t playerId, const wchar_t* playerName)
{
    dotnet_SendPlayerShopItemListRequest(this->GetHandle(), playerId, playerName);
}

void PacketFunctions_ClientToServer::SendPlayerShopItemBuyRequest(uint16_t playerId, const wchar_t* playerName, BYTE itemSlot)
{
    dotnet_SendPlayerShopItemBuyRequest(this->GetHandle(), playerId, playerName, itemSlot);
}

void PacketFunctions_ClientToServer::SendPlayerShopCloseOther(uint16_t playerId, const wchar_t* playerName)
{
    dotnet_SendPlayerShopCloseOther(this->GetHandle(), playerId, playerName);
}

void PacketFunctions_ClientToServer::SendPickupItemRequest(uint16_t itemId)
{
    dotnet_SendPickupItemRequest(this->GetHandle(), itemId);
}

void PacketFunctions_ClientToServer::SendPickupItemRequest075(uint16_t itemId)
{
    dotnet_SendPickupItemRequest075(this->GetHandle(), itemId);
}

void PacketFunctions_ClientToServer::SendDropItemRequest(BYTE targetX, BYTE targetY, BYTE itemSlot)
{
    dotnet_SendDropItemRequest(this->GetHandle(), targetX, targetY, itemSlot);
}

void PacketFunctions_ClientToServer::SendItemMoveRequest(uint32_t fromStorage, BYTE fromSlot, const BYTE* itemData, uint32_t itemDataByteLength, uint32_t toStorage, BYTE toSlot)
{
    dotnet_SendItemMoveRequest(this->GetHandle(), fromStorage, fromSlot, itemData, itemDataByteLength, toStorage, toSlot);
}

void PacketFunctions_ClientToServer::SendConsumeItemRequest(BYTE itemSlot, BYTE targetSlot, uint32_t fruitConsumption)
{
    dotnet_SendConsumeItemRequest(this->GetHandle(), itemSlot, targetSlot, fruitConsumption);
}

void PacketFunctions_ClientToServer::SendConsumeItemRequest075(BYTE itemSlot, BYTE targetSlot)
{
    dotnet_SendConsumeItemRequest075(this->GetHandle(), itemSlot, targetSlot);
}

void PacketFunctions_ClientToServer::SendTalkToNpcRequest(uint16_t npcId)
{
    dotnet_SendTalkToNpcRequest(this->GetHandle(), npcId);
}

void PacketFunctions_ClientToServer::SendCloseNpcRequest()
{
    dotnet_SendCloseNpcRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendBuyItemFromNpcRequest(BYTE itemSlot)
{
    dotnet_SendBuyItemFromNpcRequest(this->GetHandle(), itemSlot);
}

void PacketFunctions_ClientToServer::SendSellItemToNpcRequest(BYTE itemSlot)
{
    dotnet_SendSellItemToNpcRequest(this->GetHandle(), itemSlot);
}

void PacketFunctions_ClientToServer::SendRepairItemRequest(BYTE itemSlot, BYTE isSelfRepair)
{
    dotnet_SendRepairItemRequest(this->GetHandle(), itemSlot, isSelfRepair);
}

void PacketFunctions_ClientToServer::SendWarpCommandRequest(uint32_t commandKey, uint16_t warpInfoIndex)
{
    dotnet_SendWarpCommandRequest(this->GetHandle(), commandKey, warpInfoIndex);
}

void PacketFunctions_ClientToServer::SendEnterGateRequest(uint16_t gateNumber, BYTE teleportTargetX, BYTE teleportTargetY)
{
    dotnet_SendEnterGateRequest(this->GetHandle(), gateNumber, teleportTargetX, teleportTargetY);
}

void PacketFunctions_ClientToServer::SendEnterGateRequest075(BYTE gateNumber, BYTE teleportTargetX, BYTE teleportTargetY)
{
    dotnet_SendEnterGateRequest075(this->GetHandle(), gateNumber, teleportTargetX, teleportTargetY);
}

void PacketFunctions_ClientToServer::SendTeleportTarget(uint16_t targetId, BYTE teleportTargetX, BYTE teleportTargetY)
{
    dotnet_SendTeleportTarget(this->GetHandle(), targetId, teleportTargetX, teleportTargetY);
}

void PacketFunctions_ClientToServer::SendServerChangeAuthentication(const BYTE* accountXor3, uint32_t accountXor3ByteLength, const BYTE* characterNameXor3, uint32_t characterNameXor3ByteLength, uint32_t authCode1, uint32_t authCode2, uint32_t authCode3, uint32_t authCode4, uint32_t tickCount, const BYTE* clientVersion, uint32_t clientVersionByteLength, const BYTE* clientSerial, uint32_t clientSerialByteLength)
{
    dotnet_SendServerChangeAuthentication(this->GetHandle(), accountXor3, accountXor3ByteLength, characterNameXor3, characterNameXor3ByteLength, authCode1, authCode2, authCode3, authCode4, tickCount, clientVersion, clientVersionByteLength, clientSerial, clientSerialByteLength);
}

void PacketFunctions_ClientToServer::SendCastleSiegeStatusRequest()
{
    dotnet_SendCastleSiegeStatusRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendCastleSiegeRegistrationRequest()
{
    dotnet_SendCastleSiegeRegistrationRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendCastleSiegeUnregisterRequest(BYTE isGivingUp)
{
    dotnet_SendCastleSiegeUnregisterRequest(this->GetHandle(), isGivingUp);
}

void PacketFunctions_ClientToServer::SendCastleSiegeRegistrationStateRequest()
{
    dotnet_SendCastleSiegeRegistrationStateRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendCastleSiegeMarkRegistration(BYTE itemIndex)
{
    dotnet_SendCastleSiegeMarkRegistration(this->GetHandle(), itemIndex);
}

void PacketFunctions_ClientToServer::SendCastleSiegeDefenseBuyRequest(uint32_t npcNumber, uint32_t npcIndex)
{
    dotnet_SendCastleSiegeDefenseBuyRequest(this->GetHandle(), npcNumber, npcIndex);
}

void PacketFunctions_ClientToServer::SendCastleSiegeDefenseRepairRequest(uint32_t npcNumber, uint32_t npcIndex)
{
    dotnet_SendCastleSiegeDefenseRepairRequest(this->GetHandle(), npcNumber, npcIndex);
}

void PacketFunctions_ClientToServer::SendCastleSiegeDefenseUpgradeRequest(uint32_t npcNumber, uint32_t npcIndex, uint32_t npcUpgradeType, uint32_t npcUpgradeValue)
{
    dotnet_SendCastleSiegeDefenseUpgradeRequest(this->GetHandle(), npcNumber, npcIndex, npcUpgradeType, npcUpgradeValue);
}

void PacketFunctions_ClientToServer::SendCastleSiegeTaxInfoRequest()
{
    dotnet_SendCastleSiegeTaxInfoRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendCastleSiegeTaxChangeRequest(BYTE taxType, uint32_t taxRate)
{
    dotnet_SendCastleSiegeTaxChangeRequest(this->GetHandle(), taxType, taxRate);
}

void PacketFunctions_ClientToServer::SendCastleSiegeTaxMoneyWithdraw(uint32_t amount)
{
    dotnet_SendCastleSiegeTaxMoneyWithdraw(this->GetHandle(), amount);
}

void PacketFunctions_ClientToServer::SendToggleCastleGateRequest(BYTE closeState, uint16_t gateId)
{
    dotnet_SendToggleCastleGateRequest(this->GetHandle(), closeState, gateId);
}

void PacketFunctions_ClientToServer::SendCastleGuildCommand(BYTE team, BYTE positionX, BYTE positionY, BYTE command)
{
    dotnet_SendCastleGuildCommand(this->GetHandle(), team, positionX, positionY, command);
}

void PacketFunctions_ClientToServer::SendCastleSiegeHuntingZoneEntranceSetting(BYTE isPublic)
{
    dotnet_SendCastleSiegeHuntingZoneEntranceSetting(this->GetHandle(), isPublic);
}

void PacketFunctions_ClientToServer::SendCastleSiegeGateListRequest()
{
    dotnet_SendCastleSiegeGateListRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendCastleSiegeStatueListRequest()
{
    dotnet_SendCastleSiegeStatueListRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendCastleSiegeRegisteredGuildsListRequest()
{
    dotnet_SendCastleSiegeRegisteredGuildsListRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendCastleOwnerListRequest()
{
    dotnet_SendCastleOwnerListRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendFireCatapultRequest(uint16_t catapultId, BYTE targetAreaIndex)
{
    dotnet_SendFireCatapultRequest(this->GetHandle(), catapultId, targetAreaIndex);
}

void PacketFunctions_ClientToServer::SendWeaponExplosionRequest(uint16_t catapultId)
{
    dotnet_SendWeaponExplosionRequest(this->GetHandle(), catapultId);
}

void PacketFunctions_ClientToServer::SendGuildLogoOfCastleOwnerRequest()
{
    dotnet_SendGuildLogoOfCastleOwnerRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendCastleSiegeHuntingZoneEnterRequest(uint32_t money)
{
    dotnet_SendCastleSiegeHuntingZoneEnterRequest(this->GetHandle(), money);
}

void PacketFunctions_ClientToServer::SendCrywolfInfoRequest()
{
    dotnet_SendCrywolfInfoRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendCrywolfContractRequest(uint16_t statueId)
{
    dotnet_SendCrywolfContractRequest(this->GetHandle(), statueId);
}

void PacketFunctions_ClientToServer::SendCrywolfChaosRateBenefitRequest()
{
    dotnet_SendCrywolfChaosRateBenefitRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendWhiteAngelItemRequest()
{
    dotnet_SendWhiteAngelItemRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendEnterOnWerewolfRequest()
{
    dotnet_SendEnterOnWerewolfRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendEnterOnGatekeeperRequest()
{
    dotnet_SendEnterOnGatekeeperRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendLeoHelperItemRequest()
{
    dotnet_SendLeoHelperItemRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendMoveToDeviasBySnowmanRequest()
{
    dotnet_SendMoveToDeviasBySnowmanRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendSantaClausItemRequest()
{
    dotnet_SendSantaClausItemRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendKanturuInfoRequest()
{
    dotnet_SendKanturuInfoRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendKanturuEnterRequest()
{
    dotnet_SendKanturuEnterRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendRaklionStateInfoRequest()
{
    dotnet_SendRaklionStateInfoRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendCashShopPointInfoRequest()
{
    dotnet_SendCashShopPointInfoRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendCashShopOpenState(BYTE isClosed)
{
    dotnet_SendCashShopOpenState(this->GetHandle(), isClosed);
}

void PacketFunctions_ClientToServer::SendCashShopItemBuyRequest(uint32_t packageMainIndex, uint32_t category, uint32_t productMainIndex, uint16_t itemIndex, uint32_t coinIndex, BYTE mileageFlag)
{
    dotnet_SendCashShopItemBuyRequest(this->GetHandle(), packageMainIndex, category, productMainIndex, itemIndex, coinIndex, mileageFlag);
}

void PacketFunctions_ClientToServer::SendCashShopItemGiftRequest(uint32_t packageMainIndex, uint32_t category, uint32_t productMainIndex, uint16_t itemIndex, uint32_t coinIndex, BYTE mileageFlag, const wchar_t* giftReceiverName, const wchar_t* giftText)
{
    dotnet_SendCashShopItemGiftRequest(this->GetHandle(), packageMainIndex, category, productMainIndex, itemIndex, coinIndex, mileageFlag, giftReceiverName, giftText);
}

void PacketFunctions_ClientToServer::SendCashShopStorageListRequest(uint32_t pageIndex, BYTE inventoryType)
{
    dotnet_SendCashShopStorageListRequest(this->GetHandle(), pageIndex, inventoryType);
}

void PacketFunctions_ClientToServer::SendCashShopDeleteStorageItemRequest(uint32_t baseItemCode, uint32_t mainItemCode, BYTE productType)
{
    dotnet_SendCashShopDeleteStorageItemRequest(this->GetHandle(), baseItemCode, mainItemCode, productType);
}

void PacketFunctions_ClientToServer::SendCashShopStorageItemConsumeRequest(uint32_t baseItemCode, uint32_t mainItemCode, uint16_t itemIndex, BYTE productType)
{
    dotnet_SendCashShopStorageItemConsumeRequest(this->GetHandle(), baseItemCode, mainItemCode, itemIndex, productType);
}

void PacketFunctions_ClientToServer::SendCashShopEventItemListRequest(uint32_t categoryIndex)
{
    dotnet_SendCashShopEventItemListRequest(this->GetHandle(), categoryIndex);
}

void PacketFunctions_ClientToServer::SendUnlockVault(uint16_t pin)
{
    dotnet_SendUnlockVault(this->GetHandle(), pin);
}

void PacketFunctions_ClientToServer::SendSetVaultPin(uint16_t pin, const wchar_t* password)
{
    dotnet_SendSetVaultPin(this->GetHandle(), pin, password);
}

void PacketFunctions_ClientToServer::SendRemoveVaultPin(const wchar_t* password)
{
    dotnet_SendRemoveVaultPin(this->GetHandle(), password);
}

void PacketFunctions_ClientToServer::SendVaultClosed()
{
    dotnet_SendVaultClosed(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendVaultMoveMoneyRequest(uint32_t direction, uint32_t amount)
{
    dotnet_SendVaultMoveMoneyRequest(this->GetHandle(), direction, amount);
}

void PacketFunctions_ClientToServer::SendLahapJewelMixRequest(uint32_t operation, uint32_t item, uint32_t mixingStackSize, BYTE unmixingSourceSlot)
{
    dotnet_SendLahapJewelMixRequest(this->GetHandle(), operation, item, mixingStackSize, unmixingSourceSlot);
}

void PacketFunctions_ClientToServer::SendPartyListRequest()
{
    dotnet_SendPartyListRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendPartyPlayerKickRequest(BYTE playerIndex)
{
    dotnet_SendPartyPlayerKickRequest(this->GetHandle(), playerIndex);
}

void PacketFunctions_ClientToServer::SendPartyInviteRequest(uint16_t targetPlayerId)
{
    dotnet_SendPartyInviteRequest(this->GetHandle(), targetPlayerId);
}

void PacketFunctions_ClientToServer::SendPartyInviteResponse(BYTE accepted, uint16_t requesterId)
{
    dotnet_SendPartyInviteResponse(this->GetHandle(), accepted, requesterId);
}

void PacketFunctions_ClientToServer::SendWalkRequest(BYTE sourceX, BYTE sourceY, BYTE stepCount, BYTE targetRotation, const BYTE* directions, uint32_t directionsByteLength)
{
    dotnet_SendWalkRequest(this->GetHandle(), sourceX, sourceY, stepCount, targetRotation, directions, directionsByteLength);
}

void PacketFunctions_ClientToServer::SendWalkRequest075(BYTE sourceX, BYTE sourceY, BYTE stepCount, BYTE targetRotation, const BYTE* directions, uint32_t directionsByteLength)
{
    dotnet_SendWalkRequest075(this->GetHandle(), sourceX, sourceY, stepCount, targetRotation, directions, directionsByteLength);
}

void PacketFunctions_ClientToServer::SendInstantMoveRequest(BYTE targetX, BYTE targetY)
{
    dotnet_SendInstantMoveRequest(this->GetHandle(), targetX, targetY);
}

void PacketFunctions_ClientToServer::SendAnimationRequest(BYTE rotation, BYTE animationNumber)
{
    dotnet_SendAnimationRequest(this->GetHandle(), rotation, animationNumber);
}

void PacketFunctions_ClientToServer::SendRequestCharacterList(BYTE language)
{
    dotnet_SendRequestCharacterList(this->GetHandle(), language);
}

void PacketFunctions_ClientToServer::SendCreateCharacter(const wchar_t* name, uint32_t class_)
{
    dotnet_SendCreateCharacter(this->GetHandle(), name, class_);
}

void PacketFunctions_ClientToServer::SendDeleteCharacter(const wchar_t* name, const wchar_t* securityCode)
{
    dotnet_SendDeleteCharacter(this->GetHandle(), name, securityCode);
}

void PacketFunctions_ClientToServer::SendSelectCharacter(const wchar_t* name)
{
    dotnet_SendSelectCharacter(this->GetHandle(), name);
}

void PacketFunctions_ClientToServer::SendFocusCharacter(const wchar_t* name)
{
    dotnet_SendFocusCharacter(this->GetHandle(), name);
}

void PacketFunctions_ClientToServer::SendIncreaseCharacterStatPoint(uint32_t statType)
{
    dotnet_SendIncreaseCharacterStatPoint(this->GetHandle(), statType);
}

void PacketFunctions_ClientToServer::SendInventoryRequest()
{
    dotnet_SendInventoryRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendClientReadyAfterMapChange()
{
    dotnet_SendClientReadyAfterMapChange(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendSaveKeyConfiguration(const BYTE* configuration, uint32_t configurationByteLength)
{
    dotnet_SendSaveKeyConfiguration(this->GetHandle(), configuration, configurationByteLength);
}

void PacketFunctions_ClientToServer::SendAddMasterSkillPoint(uint16_t skillId)
{
    dotnet_SendAddMasterSkillPoint(this->GetHandle(), skillId);
}

void PacketFunctions_ClientToServer::SendHitRequest(uint16_t targetId, BYTE attackAnimation, BYTE lookingDirection)
{
    dotnet_SendHitRequest(this->GetHandle(), targetId, attackAnimation, lookingDirection);
}

void PacketFunctions_ClientToServer::SendTargetedSkill(uint16_t skillId, uint16_t targetId)
{
    dotnet_SendTargetedSkill(this->GetHandle(), skillId, targetId);
}

void PacketFunctions_ClientToServer::SendTargetedSkill075(BYTE skillIndex, uint16_t targetId)
{
    dotnet_SendTargetedSkill075(this->GetHandle(), skillIndex, targetId);
}

void PacketFunctions_ClientToServer::SendTargetedSkill095(BYTE skillIndex, uint16_t targetId)
{
    dotnet_SendTargetedSkill095(this->GetHandle(), skillIndex, targetId);
}

void PacketFunctions_ClientToServer::SendMagicEffectCancelRequest(uint16_t skillId, uint16_t playerId)
{
    dotnet_SendMagicEffectCancelRequest(this->GetHandle(), skillId, playerId);
}

void PacketFunctions_ClientToServer::SendAreaSkill(uint16_t skillId, BYTE targetX, BYTE targetY, BYTE rotation, uint16_t extraTargetId, BYTE animationCounter)
{
    dotnet_SendAreaSkill(this->GetHandle(), skillId, targetX, targetY, rotation, extraTargetId, animationCounter);
}

void PacketFunctions_ClientToServer::SendAreaSkill075(BYTE skillIndex, BYTE targetX, BYTE targetY, BYTE rotation)
{
    dotnet_SendAreaSkill075(this->GetHandle(), skillIndex, targetX, targetY, rotation);
}

void PacketFunctions_ClientToServer::SendAreaSkill095(BYTE skillIndex, BYTE targetX, BYTE targetY, BYTE rotation)
{
    dotnet_SendAreaSkill095(this->GetHandle(), skillIndex, targetX, targetY, rotation);
}

void PacketFunctions_ClientToServer::SendRageAttackRequest(uint16_t skillId, uint16_t targetId)
{
    dotnet_SendRageAttackRequest(this->GetHandle(), skillId, targetId);
}

void PacketFunctions_ClientToServer::SendRageAttackRangeRequest(uint16_t skillId, uint16_t targetId)
{
    dotnet_SendRageAttackRangeRequest(this->GetHandle(), skillId, targetId);
}

void PacketFunctions_ClientToServer::SendTradeCancel()
{
    dotnet_SendTradeCancel(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendTradeButtonStateChange(uint32_t newState)
{
    dotnet_SendTradeButtonStateChange(this->GetHandle(), newState);
}

void PacketFunctions_ClientToServer::SendTradeRequest(uint16_t playerId)
{
    dotnet_SendTradeRequest(this->GetHandle(), playerId);
}

void PacketFunctions_ClientToServer::SendTradeRequestResponse(BYTE tradeAccepted)
{
    dotnet_SendTradeRequestResponse(this->GetHandle(), tradeAccepted);
}

void PacketFunctions_ClientToServer::SendSetTradeMoney(uint32_t amount)
{
    dotnet_SendSetTradeMoney(this->GetHandle(), amount);
}

void PacketFunctions_ClientToServer::SendLetterDeleteRequest(uint16_t letterIndex)
{
    dotnet_SendLetterDeleteRequest(this->GetHandle(), letterIndex);
}

void PacketFunctions_ClientToServer::SendLetterListRequest()
{
    dotnet_SendLetterListRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendLetterSendRequest(uint32_t letterId, const wchar_t* receiver, const wchar_t* title, BYTE rotation, BYTE animation, uint16_t messageLength, const wchar_t* message)
{
    dotnet_SendLetterSendRequest(this->GetHandle(), letterId, receiver, title, rotation, animation, messageLength, message);
}

void PacketFunctions_ClientToServer::SendLetterReadRequest(uint16_t letterIndex)
{
    dotnet_SendLetterReadRequest(this->GetHandle(), letterIndex);
}

void PacketFunctions_ClientToServer::SendGuildKickPlayerRequest(const wchar_t* playerName, const wchar_t* securityCode)
{
    dotnet_SendGuildKickPlayerRequest(this->GetHandle(), playerName, securityCode);
}

void PacketFunctions_ClientToServer::SendGuildJoinRequest(uint16_t guildMasterPlayerId)
{
    dotnet_SendGuildJoinRequest(this->GetHandle(), guildMasterPlayerId);
}

void PacketFunctions_ClientToServer::SendGuildJoinResponse(BYTE accepted, uint16_t requesterId)
{
    dotnet_SendGuildJoinResponse(this->GetHandle(), accepted, requesterId);
}

void PacketFunctions_ClientToServer::SendGuildListRequest()
{
    dotnet_SendGuildListRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendGuildCreateRequest(const wchar_t* guildName, const BYTE* guildEmblem, uint32_t guildEmblemByteLength)
{
    dotnet_SendGuildCreateRequest(this->GetHandle(), guildName, guildEmblem, guildEmblemByteLength);
}

void PacketFunctions_ClientToServer::SendGuildCreateRequest075(const wchar_t* guildName, const BYTE* guildEmblem, uint32_t guildEmblemByteLength)
{
    dotnet_SendGuildCreateRequest075(this->GetHandle(), guildName, guildEmblem, guildEmblemByteLength);
}

void PacketFunctions_ClientToServer::SendGuildMasterAnswer(BYTE showCreationDialog)
{
    dotnet_SendGuildMasterAnswer(this->GetHandle(), showCreationDialog);
}

void PacketFunctions_ClientToServer::SendCancelGuildCreation()
{
    dotnet_SendCancelGuildCreation(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendGuildWarResponse(BYTE accepted)
{
    dotnet_SendGuildWarResponse(this->GetHandle(), accepted);
}

void PacketFunctions_ClientToServer::SendGuildInfoRequest(uint32_t guildId)
{
    dotnet_SendGuildInfoRequest(this->GetHandle(), guildId);
}

void PacketFunctions_ClientToServer::SendGuildRoleAssignRequest(uint32_t role, const wchar_t* playerName, BYTE type)
{
    dotnet_SendGuildRoleAssignRequest(this->GetHandle(), role, playerName, type);
}

void PacketFunctions_ClientToServer::SendGuildTypeChangeRequest(BYTE guildType)
{
    dotnet_SendGuildTypeChangeRequest(this->GetHandle(), guildType);
}

void PacketFunctions_ClientToServer::SendGuildRelationshipChangeRequest(uint32_t relationshipType, uint32_t requestType, uint16_t targetPlayerId)
{
    dotnet_SendGuildRelationshipChangeRequest(this->GetHandle(), relationshipType, requestType, targetPlayerId);
}

void PacketFunctions_ClientToServer::SendGuildRelationshipChangeResponse(uint32_t relationshipType, uint32_t requestType, BYTE response, uint16_t targetPlayerId)
{
    dotnet_SendGuildRelationshipChangeResponse(this->GetHandle(), relationshipType, requestType, response, targetPlayerId);
}

void PacketFunctions_ClientToServer::SendRequestAllianceList()
{
    dotnet_SendRequestAllianceList(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendRemoveAllianceGuildRequest(const wchar_t* guildName)
{
    dotnet_SendRemoveAllianceGuildRequest(this->GetHandle(), guildName);
}

void PacketFunctions_ClientToServer::SendPingResponse()
{
    dotnet_SendPingResponse(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendItemRepair(BYTE inventoryItemSlot)
{
    dotnet_SendItemRepair(this->GetHandle(), inventoryItemSlot);
}

void PacketFunctions_ClientToServer::SendChaosMachineMixRequest(uint32_t mixType, BYTE socketSlot)
{
    dotnet_SendChaosMachineMixRequest(this->GetHandle(), mixType, socketSlot);
}

void PacketFunctions_ClientToServer::SendCraftingDialogCloseRequest()
{
    dotnet_SendCraftingDialogCloseRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendFriendListRequest()
{
    dotnet_SendFriendListRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendFriendAddRequest(const wchar_t* friendName)
{
    dotnet_SendFriendAddRequest(this->GetHandle(), friendName);
}

void PacketFunctions_ClientToServer::SendFriendDelete(const wchar_t* friendName)
{
    dotnet_SendFriendDelete(this->GetHandle(), friendName);
}

void PacketFunctions_ClientToServer::SendChatRoomCreateRequest(const wchar_t* friendName)
{
    dotnet_SendChatRoomCreateRequest(this->GetHandle(), friendName);
}

void PacketFunctions_ClientToServer::SendFriendAddResponse(BYTE accepted, const wchar_t* friendRequesterName)
{
    dotnet_SendFriendAddResponse(this->GetHandle(), accepted, friendRequesterName);
}

void PacketFunctions_ClientToServer::SendSetFriendOnlineState(BYTE onlineState)
{
    dotnet_SendSetFriendOnlineState(this->GetHandle(), onlineState);
}

void PacketFunctions_ClientToServer::SendChatRoomInvitationRequest(const wchar_t* friendName, uint16_t roomId, uint32_t requestId)
{
    dotnet_SendChatRoomInvitationRequest(this->GetHandle(), friendName, roomId, requestId);
}

void PacketFunctions_ClientToServer::SendLegacyQuestStateRequest()
{
    dotnet_SendLegacyQuestStateRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendLegacyQuestStateSetRequest(BYTE questNumber, uint32_t newState)
{
    dotnet_SendLegacyQuestStateSetRequest(this->GetHandle(), questNumber, newState);
}

void PacketFunctions_ClientToServer::SendPetCommandRequest(uint32_t petType, uint32_t commandMode, uint16_t targetId)
{
    dotnet_SendPetCommandRequest(this->GetHandle(), petType, commandMode, targetId);
}

void PacketFunctions_ClientToServer::SendPetInfoRequest(uint32_t pet, uint32_t storage, BYTE itemSlot)
{
    dotnet_SendPetInfoRequest(this->GetHandle(), pet, storage, itemSlot);
}

void PacketFunctions_ClientToServer::SendIllusionTempleEnterRequest(BYTE mapNumber, BYTE itemSlot)
{
    dotnet_SendIllusionTempleEnterRequest(this->GetHandle(), mapNumber, itemSlot);
}

void PacketFunctions_ClientToServer::SendIllusionTempleSkillRequest(uint16_t skillNumber, BYTE targetObjectIndex, BYTE distance)
{
    dotnet_SendIllusionTempleSkillRequest(this->GetHandle(), skillNumber, targetObjectIndex, distance);
}

void PacketFunctions_ClientToServer::SendIllusionTempleRewardRequest()
{
    dotnet_SendIllusionTempleRewardRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendLuckyCoinCountRequest()
{
    dotnet_SendLuckyCoinCountRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendLuckyCoinRegistrationRequest()
{
    dotnet_SendLuckyCoinRegistrationRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendLuckyCoinExchangeRequest(uint32_t coinCount)
{
    dotnet_SendLuckyCoinExchangeRequest(this->GetHandle(), coinCount);
}

void PacketFunctions_ClientToServer::SendDoppelgangerEnterRequest(BYTE ticketItemSlot)
{
    dotnet_SendDoppelgangerEnterRequest(this->GetHandle(), ticketItemSlot);
}

void PacketFunctions_ClientToServer::SendEnterMarketPlaceRequest()
{
    dotnet_SendEnterMarketPlaceRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendMuHelperStatusChangeRequest(BYTE pauseStatus)
{
    dotnet_SendMuHelperStatusChangeRequest(this->GetHandle(), pauseStatus);
}

void PacketFunctions_ClientToServer::SendMuHelperSaveDataRequest(const BYTE* helperData, uint32_t helperDataByteLength)
{
    dotnet_SendMuHelperSaveDataRequest(this->GetHandle(), helperData, helperDataByteLength);
}

void PacketFunctions_ClientToServer::SendQuestSelectRequest(uint16_t questNumber, uint16_t questGroup, BYTE selectedTextIndex)
{
    dotnet_SendQuestSelectRequest(this->GetHandle(), questNumber, questGroup, selectedTextIndex);
}

void PacketFunctions_ClientToServer::SendQuestProceedRequest(uint16_t questNumber, uint16_t questGroup, uint32_t proceedAction)
{
    dotnet_SendQuestProceedRequest(this->GetHandle(), questNumber, questGroup, proceedAction);
}

void PacketFunctions_ClientToServer::SendQuestCompletionRequest(uint16_t questNumber, uint16_t questGroup)
{
    dotnet_SendQuestCompletionRequest(this->GetHandle(), questNumber, questGroup);
}

void PacketFunctions_ClientToServer::SendQuestCancelRequest(uint16_t questNumber, uint16_t questGroup)
{
    dotnet_SendQuestCancelRequest(this->GetHandle(), questNumber, questGroup);
}

void PacketFunctions_ClientToServer::SendQuestClientActionRequest(uint16_t questNumber, uint16_t questGroup)
{
    dotnet_SendQuestClientActionRequest(this->GetHandle(), questNumber, questGroup);
}

void PacketFunctions_ClientToServer::SendActiveQuestListRequest()
{
    dotnet_SendActiveQuestListRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendQuestStateRequest(uint16_t questNumber, uint16_t questGroup)
{
    dotnet_SendQuestStateRequest(this->GetHandle(), questNumber, questGroup);
}

void PacketFunctions_ClientToServer::SendEventQuestStateListRequest()
{
    dotnet_SendEventQuestStateListRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendAvailableQuestsRequest()
{
    dotnet_SendAvailableQuestsRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendNpcBuffRequest()
{
    dotnet_SendNpcBuffRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendEnterEmpireGuardianEvent(BYTE itemSlot)
{
    dotnet_SendEnterEmpireGuardianEvent(this->GetHandle(), itemSlot);
}

void PacketFunctions_ClientToServer::SendGensJoinRequest(uint32_t gensType)
{
    dotnet_SendGensJoinRequest(this->GetHandle(), gensType);
}

void PacketFunctions_ClientToServer::SendGensLeaveRequest()
{
    dotnet_SendGensLeaveRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendGensRewardRequest(uint32_t gensType)
{
    dotnet_SendGensRewardRequest(this->GetHandle(), gensType);
}

void PacketFunctions_ClientToServer::SendGensRankingRequest()
{
    dotnet_SendGensRankingRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendDevilSquareEnterRequest(BYTE squareLevel, BYTE ticketItemInventoryIndex)
{
    dotnet_SendDevilSquareEnterRequest(this->GetHandle(), squareLevel, ticketItemInventoryIndex);
}

void PacketFunctions_ClientToServer::SendMiniGameOpeningStateRequest(uint32_t eventType, BYTE eventLevel)
{
    dotnet_SendMiniGameOpeningStateRequest(this->GetHandle(), eventType, eventLevel);
}

void PacketFunctions_ClientToServer::SendEventChipRegistrationRequest(BYTE type, BYTE itemIndex)
{
    dotnet_SendEventChipRegistrationRequest(this->GetHandle(), type, itemIndex);
}

void PacketFunctions_ClientToServer::SendMutoNumberRequest()
{
    dotnet_SendMutoNumberRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendEventChipExitDialog()
{
    dotnet_SendEventChipExitDialog(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendEventChipExchangeRequest(BYTE type)
{
    dotnet_SendEventChipExchangeRequest(this->GetHandle(), type);
}

void PacketFunctions_ClientToServer::SendServerImmigrationRequest(const wchar_t* securityCode)
{
    dotnet_SendServerImmigrationRequest(this->GetHandle(), securityCode);
}

void PacketFunctions_ClientToServer::SendLuckyNumberRequest(const wchar_t* serial1, const wchar_t* serial2, const wchar_t* serial3)
{
    dotnet_SendLuckyNumberRequest(this->GetHandle(), serial1, serial2, serial3);
}

void PacketFunctions_ClientToServer::SendBloodCastleEnterRequest(BYTE castleLevel, BYTE ticketItemInventoryIndex)
{
    dotnet_SendBloodCastleEnterRequest(this->GetHandle(), castleLevel, ticketItemInventoryIndex);
}

void PacketFunctions_ClientToServer::SendMiniGameEventCountRequest(uint32_t miniGame)
{
    dotnet_SendMiniGameEventCountRequest(this->GetHandle(), miniGame);
}

void PacketFunctions_ClientToServer::SendChaosCastleEnterRequest(BYTE castleLevel, BYTE ticketItemInventoryIndex)
{
    dotnet_SendChaosCastleEnterRequest(this->GetHandle(), castleLevel, ticketItemInventoryIndex);
}

void PacketFunctions_ClientToServer::SendChaosCastlePositionSet(BYTE positionX, BYTE positionY)
{
    dotnet_SendChaosCastlePositionSet(this->GetHandle(), positionX, positionY);
}

void PacketFunctions_ClientToServer::SendDuelStartRequest(uint16_t playerId, const wchar_t* playerName)
{
    dotnet_SendDuelStartRequest(this->GetHandle(), playerId, playerName);
}

void PacketFunctions_ClientToServer::SendDuelStartResponse(BYTE response, uint16_t playerId, const wchar_t* playerName)
{
    dotnet_SendDuelStartResponse(this->GetHandle(), response, playerId, playerName);
}

void PacketFunctions_ClientToServer::SendDuelStopRequest()
{
    dotnet_SendDuelStopRequest(this->GetHandle());
}

void PacketFunctions_ClientToServer::SendDuelChannelJoinRequest(BYTE channelId)
{
    dotnet_SendDuelChannelJoinRequest(this->GetHandle(), channelId);
}

void PacketFunctions_ClientToServer::SendDuelChannelQuitRequest(BYTE channelId)
{
    dotnet_SendDuelChannelQuitRequest(this->GetHandle(), channelId);
}