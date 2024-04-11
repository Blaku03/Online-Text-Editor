using System;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

namespace Editor.Utilities;

public static class ServerListener
{
    public static async Task ListenServer(Socket socketToListen, Views.Editor editor, CancellationToken cancellationToken)
    {
        while (true)
        {
            if (cancellationToken.IsCancellationRequested)
            {
                break;
            }
            var buffer = new byte[1024];
            var bytesRead = await socketToListen.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None);
            var message = System.Text.Encoding.ASCII.GetString(buffer, 0, bytesRead);

            Console.WriteLine($"Received message: {message}");
        }
    }
} 