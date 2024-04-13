using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Editor.Utilities;

public class Paragraph
{
    internal StringBuilder Content { get; set; } = new();
    public bool IsLocked { get; set; }

    private const string LockedString = " (locked)";
    
    public Guid Id { get; } = Guid.NewGuid();

    public static LinkedList<Paragraph> GenerateFromText(string fileContent,
        LinkedList<Paragraph>? currentParagraphs = null)
    {
        var paragraphsFile = fileContent.Split("\n");

        // Remove \0 from the last paragraph
        paragraphsFile[^1] = paragraphsFile[^1].TrimEnd('\0');

        var paragraphsArr = new LinkedList<Paragraph>();
        for (var i = 0; i < paragraphsFile.Length; i++)
        {
            paragraphsArr.AddLast(new Paragraph { Content = new StringBuilder(paragraphsFile.ElementAt(i)) });
            if (currentParagraphs != null)
            {
                paragraphsArr.ElementAt(i).IsLocked = currentParagraphs.ElementAt(i).IsLocked;

                // Delete LockedSymbol from the content
                if (paragraphsArr.ElementAt(i).IsLocked)
                {
                    paragraphsArr.ElementAt(i).Content.Remove(
                        paragraphsArr.ElementAt(i).Content.Length - LockedString.Length,
                        LockedString.Length);
                }
            }
        }

        return paragraphsArr;
    }

    public static string GetContent(LinkedList<Paragraph>? paragraphs)
    {
        if (paragraphs == null) return "";
        var content = new StringBuilder();
        for (int i = 0; i < paragraphs.Count; i++)
        {
            var currentParagraph = paragraphs.ElementAt(i);
            if (currentParagraph.IsLocked && !currentParagraph.Content.ToString().EndsWith(LockedString))
            {
                var contentWithLock = new StringBuilder(currentParagraph.Content.ToString().TrimEnd('\n'));
                contentWithLock.Append(LockedString);
                content.Append(contentWithLock);
            }
            else
            {
                content.Append(currentParagraph.Content);
            }

            // Add \n at the end of each paragraph, except the last one
            if (i != paragraphs.Count - 1)
            {
                content.Append('\n');
            }
        }
        return content.ToString();
    }

    public static bool CaretAtTheEndOfLine(Paragraph paragraph, int caretColumn)
    {
        var paragraphLength = paragraph.Content.Length + 1;
        paragraphLength += paragraph.IsLocked ? LockedString.Length : 0;
        return caretColumn == paragraphLength;
    }
}