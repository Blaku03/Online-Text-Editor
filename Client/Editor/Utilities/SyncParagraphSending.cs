using System;
using System.Collections.Generic;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Editor.Utilities;

public static class SyncParagraphSending
{
    private static Dictionary<Guid, StringBuilder?> _lastSentParagraphs = new();
    public static async Task SendDataToTheServer(Socket socketToListen, Views.Editor editor, CancellationToken cancellationToken)
    {
        while (true)
        {
            if (cancellationToken.IsCancellationRequested)
            {
                break;
            }
            
            var currentParagraphNumber = editor.GetCaretLine();
            var currentParagraph = editor.GetParagraph(currentParagraphNumber);
            var content = currentParagraph?.Content;

            if (!_lastSentParagraphs.ContainsKey(currentParagraph!.Id))
            {
                _lastSentParagraphs.Add(currentParagraph.Id, new StringBuilder());
            }
            
            if (!currentParagraph.IsLocked) 
            {
                if (!_lastSentParagraphs[currentParagraph.Id]!.Equals(content))
                {
                    var data = $"{Views.Editor.SyncParagraphProtocolId},{currentParagraphNumber},{content}";
                    await socketToListen.SendAsync(System.Text.Encoding.ASCII.GetBytes(data));
                    _lastSentParagraphs[currentParagraph.Id] = content;
                }
            }
            await Task.Delay(500, cancellationToken);
        }
    }
}