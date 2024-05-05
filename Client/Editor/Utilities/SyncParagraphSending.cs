using System;
using System.Collections.Generic;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Editor.Utilities;

public static class SyncParagraphSending
{
    private static readonly Dictionary<Guid, StringBuilder?> LastSentParagraphs = new();

    public static async Task SendDataToTheServer(Socket socketToListen, Views.Editor editor,
        CancellationToken cancellationToken)
    {
        while (true)
        {
            if (cancellationToken.IsCancellationRequested)
            {
                break;
            }

            var currentParagraphNumber = editor.CaretLine;
            var currentParagraph = editor.GetParagraph(currentParagraphNumber);
            var content = currentParagraph?.Content;

            if (!LastSentParagraphs.ContainsKey(currentParagraph!.Id) && !currentParagraph.IsLocked)
            {
                LastSentParagraphs.Add(currentParagraph.Id, new StringBuilder());
            }

            if (!currentParagraph.IsLocked)
            {
                if (!LastSentParagraphs[currentParagraph.Id]!.Equals(content))
                {
                    var modifiedContent = content! + "\n";
                    var data =
                        $"{(int)Views.Editor.ProtocolId.SyncParagraph},{currentParagraphNumber},{modifiedContent}";
                    await socketToListen.SendAsync(Encoding.ASCII.GetBytes(data));
                    LastSentParagraphs[currentParagraph.Id] = content;
                }
            }
            await Task.Delay(500, cancellationToken);
        }
    }
}