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
        var buffer = new byte[10024];

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
                var metadataArray = message.Split(',', 3);
                var protocolId = (Views.Editor.ProtocolId)int.Parse(metadataArray[0]);
                switch (protocolId)
                {
                    case Views.Editor.ProtocolId.SyncParagraph:
                        var paragraphNumber = int.Parse(metadataArray[1]);
                        var paragraphContent = metadataArray[2];
                        var content = new StringBuilder(paragraphContent);
                        // Remove \n from the end of the content
                        content.Remove(content.Length - 1, 1);
                        Console.WriteLine($"Received paragraph {paragraphNumber} with content: {paragraphContent}");
                        editor.LockParagraph(paragraphNumber);
                        editor.UpdateParagraph(paragraphNumber, content);
                        editor.UpdateLockedUsers();
                        break;
                    case Views.Editor.ProtocolId.UnlockParagraph:
                        paragraphNumber = int.Parse(metadataArray[1]);
                        Console.WriteLine($"Received unlock paragraph {paragraphNumber}");
                        editor.UnlockParagraph(paragraphNumber);
                        editor.UpdateParagraph(paragraphNumber, new StringBuilder(), true);
                        editor.UpdateLockedUsers(deletes: true);
                        break;
                    case Views.Editor.ProtocolId.AsyncDeleteParagraph:
                        paragraphNumber = int.Parse(metadataArray[1]);
                        editor.DeleteParagraph(paragraphNumber); //delete paragraph from message
                        editor.LockParagraph(paragraphNumber - 1); //lock paragraph above
                        editor.UpdateLockedUsers();
                        break;
                    case Views.Editor.ProtocolId.AsyncNewParagraph:
                        paragraphNumber = int.Parse(metadataArray[1]);
                        paragraphContent = metadataArray[2]; //previous paragraph content to update
                        content = new StringBuilder(paragraphContent.TrimEnd('\n'));
                        editor.UnlockParagraph(paragraphNumber); //unlock previous paragraph
                        editor.UpdateParagraph(paragraphNumber, content); //update content of previous paragraph
                        editor.AddNewParagraphAfter(paragraphNumber); //add new paragraph
                        editor.LockParagraph(paragraphNumber + 1); //lock new paragraph
                        editor.UpdateLockedUsers(deletes: true);
                        break;
                    case Views.Editor.ProtocolId.AddKnownWord:
                        editor.AddWordToDictionary(metadataArray[1]);
                        break;
                    case Views.Editor.ProtocolId.UnlockParagraphWithInactiveUser:
                        paragraphNumber = int.Parse(metadataArray[1]);
                        Console.WriteLine($"Received unlock paragraph {paragraphNumber}");
                        editor.UnlockParagraph(paragraphNumber);
                        editor.UpdateLockedUsers(deletes: true);
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