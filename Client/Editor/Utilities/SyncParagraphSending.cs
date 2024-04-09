using System;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

namespace Editor.Utilities;

public static class SyncParagraphSending
{
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
            var data = $"{currentParagraphNumber},{content}";
            await socketToListen.SendAsync(System.Text.Encoding.ASCII.GetBytes(data));
            await Task.Delay(500, cancellationToken);
        }
    }
}