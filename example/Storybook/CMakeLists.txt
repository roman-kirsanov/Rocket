# Storybook: shows the components of src/Rocket/UI in a window of its own.
# The place to look at a component while working on it.
add_executable(Storybook
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/App.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/Code.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/CodeBlock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/ColorScheme.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/ColorSchemeToggle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/NavLink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/Playground.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/PlaygroundControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/PropsTable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/SegmentedControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/Select.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/Stage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/Text.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/Title.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Pages/ButtonPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Pages/CheckboxPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Pages/MaterialPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Pages/RadioboxPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Pages/SwitchPage.cpp
)

target_include_directories(Storybook
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Storybook
    PRIVATE
        Rocket::Rocket
)
