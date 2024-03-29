using System;
using System.Net.Sockets;
using System.Threading.Tasks;

namespace Editor;

public static class SocketListener
{
    public static async Task ListenForMessages(Socket socketToListen)
    {
        try
        {
            // Continuously read messages from the server
            var buffer = new byte[1024];
            while (true)
            {
                var bytesRead = await socketToListen.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None);
                var message = System.Text.Encoding.ASCII.GetString(buffer, 0, bytesRead);

                Console.WriteLine($"Received message: {message}");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error: {ex.Message}");
        }
    }
}