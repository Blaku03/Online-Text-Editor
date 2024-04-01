using System.Collections.Generic;
using System.Text;

namespace Editor;

public class Paragraph
{
    public StringBuilder Content { get; set; } = new();
    public bool IsLocked { get; set; } = false;

    public static Paragraph[] GetParagraphs(string fileContent)
    {
        var paragraphs = fileContent.Split("\n");

        // Remove \0 from the last paragraph
        paragraphs[^1] = paragraphs[^1].TrimEnd('\0');
        var paragraphsArr = new Paragraph[paragraphs.Length];
        for (var i = 0; i < paragraphs.Length; i++)
        {
            paragraphsArr[i] = new Paragraph { Content = new StringBuilder(paragraphs[i]) };
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
            if (paragraph.IsLocked)
            {
                content.Append(" (locked)");
            }
            content.Append('\n');
        }

        return content.ToString();
    }
}