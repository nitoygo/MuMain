using System.Text;
using MUnique.Client.Library;

// LoginWithTicketPacket writes the layout OpenMU's LoginWithTicket handler reads: C3 F1 10, length 72, the token as
// plain UTF-8 at 4, the tick count big-endian at 47, the client version at 51 and the serial at 56.
internal static class Program
{
    private const string Token = "AbCdEfGhIjKlMnOpQrStUvWxYz0123456789-_AbCdE";

    private static void Main()
    {
        byte[] version = [0x31, 0x30, 0x34, 0x30, 0x34];
        byte[] serial = Encoding.ASCII.GetBytes("k1Pk2jcET48mxL3b");
        var packet = new byte[LoginWithTicketPacket.Length + 8];
        Array.Fill(packet, (byte)0xEE);

        var written = LoginWithTicketPacket.Write(packet, Token, 0x01020304, version, serial);

        Expect(written == 72, $"wrote {written} bytes, expected 72");
        Expect(packet[0] == 0xC3 && packet[1] == 72 && packet[2] == 0xF1 && packet[3] == 0x10, "header is not C3 48 F1 10");
        Expect(Encoding.ASCII.GetString(packet, 4, 43) == Token, "token is not plain UTF-8 at index 4");
        Expect(packet[47] == 0x01 && packet[48] == 0x02 && packet[49] == 0x03 && packet[50] == 0x04, "tick count is not big-endian at 47");
        Expect(packet.AsSpan(51, 5).SequenceEqual(version), "client version is not at 51");
        Expect(packet.AsSpan(56, 16).SequenceEqual(serial), "client serial is not at 56");
        Expect(packet[72] == 0xEE, "wrote past the packet");

        // A shorter token is zero-padded, so no bytes of an earlier packet stay in the field.
        LoginWithTicketPacket.Write(packet, "short", 0, version, serial);
        Expect(packet.AsSpan(4 + 5, 43 - 5).IndexOfAnyExcept((byte)0) < 0, "a short token is not zero-padded");

        var tooLong = Token + "x";
        try
        {
            LoginWithTicketPacket.Write(packet, tooLong, 0, version, serial);
            Expect(false, "a token longer than the field was written");
        }
        catch (ArgumentException)
        {
        }
    }

    private static void Expect(bool condition, string failure)
    {
        if (!condition)
        {
            throw new InvalidOperationException(failure);
        }
    }
}
