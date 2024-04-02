using System;
using System.Collections.Generic;
using System.Text;

namespace Editor.Utilities;

public class Paragraph
{
    public StringBuilder Content { get; set; } = new();
    public bool IsLocked { get; set; } = false;

    public static Paragraph[] GetParagraphs(string fileContent, Paragraph[]? currentParagraphs = null)
    {
        var paragraphs = fileContent.Split("\n");
        // Remove last empty line
        if (paragraphs[^1] == "")
        {
            Array.Resize(ref paragraphs, paragraphs.Length - 1);
        }

        // Remove \0 from the last paragraph
        paragraphs[^1] = paragraphs[^1].TrimEnd('\0');
        var paragraphsArr = new Paragraph[paragraphs.Length];
        for (var i = 0; i < paragraphs.Length; i++)
        {
            paragraphsArr[i] = new Paragraph
                { Content = new StringBuilder(paragraphs[i]) };
            if (currentParagraphs != null && i < currentParagraphs.Length)
            {
                paragraphsArr[i].IsLocked = currentParagraphs[i].IsLocked;
            }
        }

        return paragraphsArr;
    }

    public static string GetContent(IEnumerable<Paragraph>? paragraphs)
    {
        if (paragraphs == null) return "";
        var content = new StringBuilder();
        foreach (var paragraph in paragraphs)
        {
            content.Append(paragraph.Content);
            if (paragraph.IsLocked && !content.ToString().EndsWith("(locked)"))
            {
                content.Append(" (locked)");
            }

            content.Append('\n');
        }

        return content.ToString();
    }
}