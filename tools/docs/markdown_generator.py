from dataclasses import dataclass, field

@dataclass
class MarkdownWriter:
    _lines: list[str] = field(default_factory=list)

    def build(self) -> str:
        return "\n".join(self._lines)

    def write_header(self, header_name: str, sublevel: int):
        if sublevel < 1:
            print("[MarkdownWriter] [write_header] Invalid sublevel, it must be greater than 1")
            return
        self._lines.append(f"{'#' * sublevel} {header_name}")

    def write_italic(self, text: str):
        self._lines.append(f"*{text}*")

    def write_bold(self, text: str):
        self._lines.append(f"**{text}**")

    def write_bold_italic(self, text: str):
        self._lines.append(f"***{text}***")

    def write_single_line_codeblock(self, code: str, language: str):
            self._lines.append(f"```{language}\n{code}\n```")

    def write_text(self, text: str):
            self._lines.append(text)

    def write_horizontal_rule(self):
        self._lines.append("---")

    def write_quote(self, text: str):
        self._lines.append(f"> {text}")

    def write_link(self, text: str, url: str):
        self._lines.append(f"[{text}]({url})")

    def write_image(self, alt: str, url: str):
        self._lines.append(f"![{alt}]({url})")

    def write_bullet(self, text: str, level: int = 0):
        self._lines.append(f"{'  ' * level}- {text}")

    def write_numbered_item(self, text: str, number: int):
        self._lines.append(f"{number}. {text}")

    def write_table(self, headers: list[str], rows: list[list[str]]):
        self._lines.append("| " + " | ".join(headers) + " |")
        self._lines.append("| " + " | ".join("---" for _ in headers) + " |")

        for row in rows:
            self._lines.append("| " + " | ".join(row) + " |")

    def write(self, text: str):
        self._lines.append(text)

    def newline(self):
        self._lines.append("")

    def clear(self):
        self._lines.clear()