// <copyright file="LoginWithTicketPacket.cs" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

namespace MUnique.Client.Library;

using System;
using System.Buffers.Binary;
using System.Text;

/// <summary>
/// Writes the LoginWithTicket packet (C3 F1 10): a login with a single-use ticket instead of a password.
/// </summary>
/// <remarks>
/// Written by hand until the MUnique.OpenMU.Network.Packets package has the packet; then the generated struct replaces it.
/// </remarks>
internal static class LoginWithTicketPacket
{
    /// <summary>The length of the packet.</summary>
    public const int Length = 72;

    /// <summary>The length of the token field: 256 bits as base64url without padding.</summary>
    public const int TokenLength = 43;

    /// <summary>The length of the client version field.</summary>
    public const int ClientVersionLength = 5;

    /// <summary>The length of the client serial field.</summary>
    public const int ClientSerialLength = 16;

    private const byte HeaderType = 0xC3;
    private const byte Code = 0xF1;
    private const byte SubCode = 0x10;
    private const int TokenOffset = 4;
    private const int TickCountOffset = 47;
    private const int ClientVersionOffset = 51;
    private const int ClientSerialOffset = 56;

    /// <summary>
    /// Writes the packet into <paramref name="packet"/>.
    /// </summary>
    /// <param name="packet">The destination, at least <see cref="Length"/> bytes.</param>
    /// <param name="token">The ticket's token, at most <see cref="TokenLength"/> UTF-8 bytes.</param>
    /// <param name="tickCount">The tick count.</param>
    /// <param name="clientVersion">The client version, <see cref="ClientVersionLength"/> bytes.</param>
    /// <param name="clientSerial">The client serial, <see cref="ClientSerialLength"/> bytes.</param>
    /// <returns>The number of bytes written.</returns>
    public static int Write(Span<byte> packet, string token, uint tickCount, ReadOnlySpan<byte> clientVersion, ReadOnlySpan<byte> clientSerial)
    {
        if (Encoding.UTF8.GetByteCount(token) > TokenLength)
        {
            throw new ArgumentException("Login ticket exceeds packet field length.", nameof(token));
        }

        packet = packet[..Length];
        packet.Clear();
        packet[0] = HeaderType;
        packet[1] = Length;
        packet[2] = Code;
        packet[3] = SubCode;

        // Plain UTF-8: the server reads the token as is, unlike the Xor3 "encrypted" password login.
        Encoding.UTF8.GetBytes(token, packet.Slice(TokenOffset, TokenLength));
        BinaryPrimitives.WriteUInt32BigEndian(packet[TickCountOffset..], tickCount);
        clientVersion[..ClientVersionLength].CopyTo(packet[ClientVersionOffset..]);
        clientSerial[..ClientSerialLength].CopyTo(packet[ClientSerialOffset..]);
        return Length;
    }
}
