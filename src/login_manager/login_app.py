# FreeNT Login Manager - Main Application
# Copyright (c) 2026, Panoc95
# BSD 3-Clause License

"""
Textual-based login manager for FreeNT.
This module provides the main login interface using Python Textual.
"""

import asyncio
from typing import Optional

try:
    from textual.app import App, ComposeResult
    from textual.containers import Container, Grid
    from textual.widgets import (
        Button,
        Header,
        Footer,
        Input,
        Label,
        Static,
    )
    from textual.screen import Screen
    from textual import events
    TEXTUAL_AVAILABLE = True
except ImportError:
    TEXTUAL_AVAILABLE = False
    print("Warning: Textual not available. Login manager requires textual>=0.1.0")


class LoginScreen(Screen[None]):
    """Main login screen for FreeNT."""

    CSS = """
    Screen {
        align: center middle;
        background: $background;
    }
    
    Grid {
        width: 80%;
        height: 60%;
        grid-size: 2;
        grid-columns: 1fr;
        grid-rows: 1fr 1fr 1fr 1fr;
        align: center middle;
    }
    
    Container {
        width: 100%;
        height: 100%;
        align: center middle;
    }
    
    Header {
        dock: top;
        background: $accent;
        color: $text;
    }
    
    Footer {
        dock: bottom;
        background: $surface;
        color: $text-muted;
    }
    
    Label {
        width: 100%;
        text-align: center;
        margin: 1 0;
    }
    
    Input {
        width: 80%;
        margin: 1 0;
    }
    
    Button {
        width: 60%;
        margin: 1 0;
    }
    
    Static {
        width: 100%;
        text-align: center;
        color: $text-error;
        margin: 1 0;
    }
    """

    def __init__(self, name: str | None = None) -> None:
        super().__init__(name=name)
        self.username_input: Optional[Input] = None
        self.password_input: Optional[Input] = None
        self.error_label: Optional[Static] = None

    def compose(self) -> ComposeResult:
        """Compose the login screen UI."""
        yield Header(show_clock=True)
        yield Grid(
            Container(
                Label("FreeNT Login", id="title"),
                id="title-container",
            ),
            Container(
                Label("Username:", id="username-label"),
                Input(
                    placeholder="Enter username",
                    id="username-input",
                ),
                id="username-container",
            ),
            Container(
                Label("Password:", id="password-label"),
                Input(
                    placeholder="Enter password",
                    password=True,
                    id="password-input",
                ),
                id="password-container",
            ),
            Container(
                Button("Login", id="login-button", variant="primary"),
                Button("Shutdown", id="shutdown-button", variant="error"),
                Button("Reboot", id="reboot-button", variant="warning"),
                id="buttons-container",
            ),
            Container(
                Static("", id="error-label"),
                id="error-container",
            ),
            id="login-grid",
        )
        yield Footer()

    def on_ready(self) -> None:
        """Called when the screen is ready."""
        self.username_input = self.query_one("#username-input", Input)
        self.password_input = self.query_one("#password-input", Input)
        self.error_label = self.query_one("#error-label", Static)

    def on_button_pressed(self, event: events.Button.Pressed) -> None:
        """Handle button press events."""
        button_id = event.button.id

        if button_id == "login-button":
            self._handle_login()
        elif button_id == "shutdown-button":
            self._handle_shutdown()
        elif button_id == "reboot-button":
            self._handle_reboot()

    def _handle_login(self) -> None:
        """Handle login button press."""
        username = self.username_input.value if self.username_input else ""
        password = self.password_input.value if self.password_input else ""

        if not username or not password:
            if self.error_label:
                self.error_label.update("Username and password are required")
            return

        # TODO: Implement actual authentication
        # For now, just show success
        if self.error_label:
            self.error_label.update("")
        
        # Simulate successful login
        self.app.exit(result=(username, password))

    def _handle_shutdown(self) -> None:
        """Handle shutdown button press."""
        # TODO: Implement shutdown functionality
        if self.error_label:
            self.error_label.update("Shutdown functionality not yet implemented")

    def _handle_reboot(self) -> None:
        """Handle reboot button press."""
        # TODO: Implement reboot functionality
        if self.error_label:
            self.error_label.update("Reboot functionality not yet implemented")


class LoginApp(App[tuple[str, str]]):
    """Main login application for FreeNT."""

    CSS = """
    App {
        background: $background 90%;
        color: $text;
    }
    
    /* Light theme colors */
    :root {
        --background: #f0f0f0;
        --surface: #e0e0e0;
        --primary: #0078d7;
        --accent: #0078d7;
        --text: #000000;
        --text-muted: #666666;
        --text-error: #d32f2f;
    }
    
    /* Dark theme colors */
    .dark {
        --background: #121212;
        --surface: #1e1e1e;
        --primary: #0078d7;
        --accent: #0078d7;
        --text: #ffffff;
        --text-muted: #aaaaaa;
        --text-error: #ff6b6b;
    }
    """

    BINDINGS = [
        ("ctrl+q", "quit", "Quit"),
        ("ctrl+d", "toggle_dark", "Toggle dark mode"),
    ]

    def __init__(self) -> None:
        super().__init__()
        self.dark_mode = False

    def on_ready(self) -> None:
        """Called when the app is ready."""
        self.push_screen(LoginScreen())

    def action_toggle_dark(self) -> None:
        """Toggle dark mode."""
        self.dark_mode = not self.dark_mode
        if self.dark_mode:
            self.add_class("dark")
        else:
            self.remove_class("dark")

    async def on_mount(self) -> None:
        """Called when the app is mounted."""
        # Set window title
        self.title = "FreeNT Login Manager"
        
        # Set window size
        self.screen.styles.width = "100%"
        self.screen.styles.height = "100%"


if __name__ == "__main__":
    if TEXTUAL_AVAILABLE:
        app = LoginApp()
        result = app.run()
        if result:
            username, password = result
            print(f"Login successful: {username}")
    else:
        print("Error: Textual is required to run the login manager.")
        print("Install it with: pip install textual")
