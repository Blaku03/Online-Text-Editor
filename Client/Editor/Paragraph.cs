using System;
using System.Linq;
using System.Text;

namespace Editor;

public class Paragraph
{
    public StringBuilder Content { get; set; } = new();
    public bool IsLocked { get; set; } = false;

    public static Paragraph[] GetParagraphs(StringBuilder fileContent)
    {
        var paragraphs = fileContent.ToString().Split("\n");
        var paragraphsArr = new Paragraph[paragraphs.Length];
        for(int i = 0; i < paragraphs.Length; i++)
        {
            paragraphsArr[i] = new Paragraph { Content = new StringBuilder(paragraphs[i]) };
        }
        return paragraphsArr;
    }

    public static String GetContent(Paragraph[] paragraphs)
    {
        var content = new StringBuilder();
        foreach (var paragraph in paragraphs)
        {
            content.Append(paragraph.Content);
            content.Append('\n');
        }

        return content.ToString();
    }
}