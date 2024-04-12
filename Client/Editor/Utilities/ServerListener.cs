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
            var metadataArray = message.Split(',');
            var protocolId = int.Parse(metadataArray[0]);

            switch (protocolId)
            {
                case Views.Editor.SyncParagraphProtocolId:
                    var paragraphNumber = int.Parse(metadataArray[1]);
                    var paragraphContent = metadataArray[2];
                    Console.WriteLine($"Received paragraph {paragraphNumber} with content: {paragraphContent}");
                    editor.UpdateParagraph(paragraphNumber, paragraphContent);
                    editor.LockParagraph(paragraphNumber);
                    break;
                case Views.Editor.UnlockParagraphProtocolId:
                    paragraphNumber = int.Parse(metadataArray[1]);
                    Console.WriteLine($"Received unlock paragraph {paragraphNumber}");
                    editor.UnlockParagraph(paragraphNumber);
                    break;
                case Views.Editor.AsyncDeleteParagraphProtocolId:
                    paragraphNumber = int.Parse(metadataArray[1]);
                    break;
                case Views.Editor.AsyncNewParagraphProtocolId:
                    var newParagraphNumber = int.Parse(metadataArray[1]);
                    var newParagraphContent = metadataArray[2];
                    break;
            }

            Console.WriteLine($"Received message: {message}");
        }
    }
}