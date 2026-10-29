#pragma once
class TextLabelClass; class MessageListClass;
namespace game {
struct GameUiFrame;
bool build_text_label(GameUiFrame&, const TextLabelClass&, bool forced);
void build_messages(GameUiFrame&, const MessageListClass&);
}
