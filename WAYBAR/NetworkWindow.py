#!/usr/bin/env python3
import subprocess
import gi
import sys

# Garante as versões corretas do GTK e Layer Shell
gi.require_version('Gtk', '3.0')
try:
    gi.require_version('GtkLayerShell', '0.1')
except ValueError:
    print("Erro: Instale 'gtk-layer-shell' ou 'libgtk-layer-shell' na sua distro.")
    sys.exit(1)

from gi.repository import Gtk, Gdk, GtkLayerShell

# Inicializa o GTK
Gtk.init()

# 1. ESTILIZAÇÃO CSS (Deixa o menu bonitinho)
style_provider = Gtk.CssProvider()
style_provider.load_from_data(b"""
    window {
        background-color: rgba(30, 30, 46, 0.95); /* Fundo escuro semi-transparente */
        border: 2px solid #89b4fa;               /* Borda azul elegante */
        border-radius: 12px;                      /* Cantos arredondados */
    }
    box {
        padding: 10px;
    }
    button {
        background: #313244;
        color: #cdd6f4;
        border: none;
        border-radius: 6px;
        padding: 8px 16px;
        font-weight: bold;
        transition: all 0.2s ease-in-out;
    }
    button:hover {
        background: #89b4fa;
        color: #11111b;
    }
""")

# Aplica o CSS globalmente na janela
Gtk.StyleContext.add_provider_for_screen(
    Gdk.Screen.get_default(),
    style_provider,
    Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
)

# 2. CRIAÇÃO DA JANELA
window = Gtk.Window()

# Inicializa o Layer Shell para controlar a posição no Wayland
GtkLayerShell.init_for_window(window)
GtkLayerShell.set_layer(window, GtkLayerShell.Layer.TOP) # Garante que fica acima de tudo

# ANCORAGEM: Dizemos para colar no TOPO (Top). 
# Para alinhar Esquerda ou Direita, mude abaixo:
GtkLayerShell.set_anchor(window, GtkLayerShell.Edge.TOP, True)
GtkLayerShell.set_anchor(window, GtkLayerShell.Edge.RIGHT, True) # Alinha na direita da tela

# Margem para não ficar colado na borda da tela ou grudado na Waybar
GtkLayerShell.set_margin(window, GtkLayerShell.Edge.TOP, 45)  # Ajuste conforme a altura da sua Waybar
GtkLayerShell.set_margin(window, GtkLayerShell.Edge.RIGHT, 20) # Distância da borda direita

# Fecha o menu ao clicar fora
def on_focus_out(widget, event):
    Gtk.main_quit()
    return True
window.connect("focus-out-event", on_focus_out)

# 3. INTERFACE (Botões)
box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)

btn_wifi_on = Gtk.Button(label="󰤨  Ligar Wifi")
btn_wifi_off = Gtk.Button(label="󰤭  Desligar Wifi")
btn_settings = Gtk.Button(label="󰢨  Configurações")

# Funções de clique
def wifi_on_clicked(widget):
    subprocess.run(["nmcli", "radio", "wifi", "on"])
    Gtk.main_quit()

def wifi_off_clicked(widget):
    subprocess.run(["nmcli", "radio", "wifi", "off"])
    Gtk.main_quit()

def settings_clicked(widget):
    subprocess.Popen(["nm-connection-editor"])
    Gtk.main_quit()

btn_wifi_on.connect("clicked", wifi_on_clicked)
btn_wifi_off.connect("clicked", wifi_off_clicked)
btn_settings.connect("clicked", settings_clicked)

# Adiciona tudo
box.add(btn_wifi_on)
box.add(btn_wifi_off)
box.add(btn_settings)
window.add(box)

window.show_all()
Gtk.main()