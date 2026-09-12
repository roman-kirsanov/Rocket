# Rocket 🚀

**Declarative native UI for modern C++.**

```cpp
void Button(std::string const& title, std::function<void()> const& onClick) {
    Node({
        .key = title,
        .padding = 8.0f,
        .borderRadius = 6.0f,
        .cursor = Cursor::Pointer,
        .background = ColorBrush{ .color = Vec4{ 0.15f, 0.15f, 0.15f, 1.0f } },
        .onEvent = [onClick](NodeEvent const& event) {
            if (event.is<MouseClickNodeEvent>()) {
                onClick();
            }
        }
    }, [&]{
        Node({
            .display = NodeDisplay::Text,
            .content = title
        });
    });
}

void Counter() {
    COMPONENT_NO_KEY

    auto& count = UseState<std::int32_t>(0);
    auto& label = UseState<std::string>();

    auto shouldUpdateLabel = UseEffect(count);
    if (shouldUpdateLabel) {
        label = std::to_string(count);
    }

    Node({
        .direction = NodeDirection::Vertical,
        .alignment = NodeAlignment::Center,
        .gapY = 16.0f
    }, [&]{
        Node({
            .display = NodeDisplay::Text,
            .content = "Count: "
        }, [&]{
            Node({
                .display = NodeDisplay::Text,
                .fontWeight = FontWeight::Bold,
                .content = label
            });
        });

        Node({
            .direction = NodeDirection::Horizontal,
            .gapX = 16.0f
        }, [&]{
            Button("Decrement", [&]{ count--; });
            Button("Increment", [&]{ count++; });
        });
    });
}
```
