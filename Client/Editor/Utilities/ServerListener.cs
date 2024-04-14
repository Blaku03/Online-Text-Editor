using System;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Editor.Utilities;

public static class ServerListener
{
    public static async Task ListenServer(Socket socketToListen, Views.Editor editor,
        CancellationToken cancellationToken)
    {
        var buffer = new byte[1024];

        while (true)
        {
            if (cancellationToken.IsCancellationRequested)
            {
                break;
            }

            var bytesRead = await socketToListen.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None);
            var message = Encoding.ASCII.GetString(buffer, 0, bytesRead);
            Console.WriteLine($"Received message: {message}");
            try
            {
                var metadataArray = message.Split(',');
                var protocolId = (Views.Editor.ProtocolId)int.Parse(metadataArray[0]);
                string? paragraphContent;
                StringBuilder? content;
                switch (protocolId)
                {
                    case Views.Editor.ProtocolId.SyncParagraph:
                        var paragraphNumber = int.Parse(metadataArray[1]);
                        paragraphContent = metadataArray[2];
                        content = new StringBuilder(paragraphContent);
                        Console.WriteLine($"Received paragraph {paragraphNumber} with content: {paragraphContent}");
                        editor.LockParagraph(paragraphNumber);
                        editor.UpdateParagraph(paragraphNumber, content);
                        break;
                    case Views.Editor.ProtocolId.UnlockParagraph:
                        paragraphNumber = int.Parse(metadataArray[1]);
                        paragraphContent = metadataArray[2];
                        content = new StringBuilder(paragraphContent.TrimEnd('\n'));
                        Console.WriteLine($"Received unlock paragraph {paragraphNumber}");
                        editor.UnlockParagraph(paragraphNumber);
                        editor.UpdateParagraph(paragraphNumber, content);
                        break;
                    case Views.Editor.ProtocolId.ChangeLineAfterMousePress:
                        paragraphNumber = int.Parse(metadataArray[1]);
                        var current = editor.GetParagraph(paragraphNumber);
                        content = new StringBuilder(current!.Content.ToString().TrimEnd('\n'));
                        editor.UnlockParagraph(paragraphNumber);
                        editor.UpdateParagraph(paragraphNumber, content);
                        break;
                    case Views.Editor.ProtocolId.AsyncDeleteParagraph:
                        paragraphNumber = int.Parse(metadataArray[1]);
                        break;
                    case Views.Editor.ProtocolId.AsyncNewParagraph:
                        var newParagraphNumber = int.Parse(metadataArray[1]);
                        var newParagraphContent = metadataArray[2];
                        break;
                }
            }
            catch (Exception e)
            {
                // ignored
            }
        }
    }
}