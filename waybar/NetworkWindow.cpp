#include <gtkmm.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <algorithm>
#include <memory>

// Função auxiliar para executar comandos e pegar o retorno
std::string exec_cmd(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

// Verifica se existe alguma interface Wi-Fi disponível no sistema
bool has_wifi_device() {
    std::string out = exec_cmd("/usr/bin/nmcli device");
    return out.find("wifi") != std::string::npos;
}

bool is_wifi_on() {
    std::string out = exec_cmd("/usr/bin/nmcli radio wifi");
    return out.find("enabled") != std::string::npos;
}

// Retorna o SSID da rede conectada atualmente (se houver)
std::string get_current_wifi() {
    std::string out = exec_cmd("/usr/bin/nmcli -t -f ACTIVE,SSID dev wifi | grep '^yes:'");
    if (!out.empty()) {
        out.erase(0, 4); // Remove o "yes:"
        out.erase(out.find_last_not_of(" \t\r\n") + 1);
        return out;
    }
    return "";
}

// Verifica se a rede precisa de senha (WPA/WEP)
bool network_needs_password(const std::string& ssid) {
    std::string cmd = "/usr/bin/nmcli -t -f SSID,SECURITY dev wifi | grep '^" + ssid + ":'";
    std::string out = exec_cmd(cmd.c_str());
    if (out.find("WPA") != std::string::npos || out.find("WEP") != std::string::npos) {
        return true;
    }
    return false;
}

std::vector<std::string> get_wifi_networks() {
    std::system("/usr/bin/nmcli device wifi rescan > /dev/null 2>&1");
    std::string out = exec_cmd("/usr/bin/nmcli -f SSID dev wifi");
    
    std::stringstream ss(out);
    std::string line;
    std::vector<std::string> networks;
    std::set<std::string> seen;
    
    bool first = true;
    while (std::getline(ss, line)) {
        if (first) { first = false; continue; }
        
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (!line.empty() && line != "--" && seen.find(line) == seen.end()) {
            seen.insert(line);
            networks.push_back(line);
            if (networks.size() >= 6) break;
        }
    }
    std::sort(networks.begin(), networks.end());
    return networks;
}

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.waybar.wifimenu");
    Gtk::Window window;
    
    GtkWindow* gtk_win = window.gobj();
    gtk_layer_init_for_window(gtk_win);
    gtk_layer_set_layer(gtk_win, GTK_LAYER_SHELL_LAYER_TOP);
    
    gtk_layer_set_anchor(gtk_win, GTK_LAYER_SHELL_EDGE_TOP, true);
    gtk_layer_set_anchor(gtk_win, GTK_LAYER_SHELL_EDGE_RIGHT, true);
    
    gtk_layer_set_margin(gtk_win, GTK_LAYER_SHELL_EDGE_TOP, 25);
    gtk_layer_set_margin(gtk_win, GTK_LAYER_SHELL_EDGE_RIGHT, 20);

    // CSS limpo e sem propriedades ilegais para o GTK
    auto css = Gtk::CssProvider::create();
    css->load_from_data(
        "window { background-color: rgba(30, 30, 46, 0.95); border: 2px solid #ffffff; border-radius: 12px; }"
        "box { padding: 12px; }"
        "label.title { color: #a6adc8; font-weight: bold; font-size: 11px; margin-top: 5px; margin-bottom: 2px; }"
        "button { background: #313244; color: #cdd6f4; border: 2px solid transparent; border-radius: 6px; padding: 8px 16px; font-weight: bold; }"
        "button:hover { background: #ffffff; color: #11111b; }"
        ".wifi-list-btn { background: #1e1e2e; font-weight: normal; border: 2px solid transparent; }"
        ".wifi-list-btn:hover { background: #45475a; color: #ffffff; }"
        ".selected { border-color: #f38ba8; background: #313244; }"
        "entry { background: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 6px; padding: 4px; padding-left: 8px; }"
        ".action-btn { background: #a6e3a1; color: #11111b; margin-left: 20px; }"
        ".disconnect { background: #f38ba8; }"
    );
    auto screen = Gdk::Screen::get_default();
    Gtk::StyleContext::add_provider_for_screen(screen, css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    Gtk::Box main_box(Gtk::ORIENTATION_VERTICAL, 8);
    
    bool has_wifi = has_wifi_device();
    bool wifi_active = has_wifi && is_wifi_on();
    std::string current_connected = get_current_wifi();

    // Se NÃO tiver placa Wi-Fi (Apenas Cabo/Fio)
    if (!has_wifi) {
        auto lbl_cable = Gtk::make_managed<Gtk::Label>("CONEXÃO CABEADA DETECTADA");
        lbl_cable->get_style_context()->add_class("title");
        main_box.pack_start(*lbl_cable);

        std::string net_enabled = exec_cmd("/usr/bin/nmcli networking");
        bool is_net_on = (net_enabled.find("enabled") != std::string::npos);

        auto btn_cable_toggle = Gtk::make_managed<Gtk::Button>(is_net_on ? "󰈀  Desligar Rede" : "󰈀  Ligar Rede");
        btn_cable_toggle->signal_clicked().connect([is_net_on]() {
            if (is_net_on) std::system("/usr/bin/nmcli networking off");
            else std::system("/usr/bin/nmcli networking on");
            Gtk::Main::quit();
        });
        main_box.pack_start(*btn_cable_toggle);
    } 
    // Se TIVER placa Wi-Fi
    else {
        auto btn_toggle = Gtk::make_managed<Gtk::Button>(wifi_active ? "󰤭  Desligar Wifi" : "󰤨  Ligar Wifi");
        btn_toggle->set_halign(Gtk::ALIGN_CENTER);
        btn_toggle->signal_clicked().connect([wifi_active]() {
            if (wifi_active) std::system("/usr/bin/nmcli radio wifi off");
            else std::system("/usr/bin/nmcli radio wifi on");
            Gtk::Main::quit();
        });
        main_box.pack_start(*btn_toggle);

        if (wifi_active) {
            auto lbl_list = Gtk::make_managed<Gtk::Label>("REDES DISPONÍVEIS:");
            lbl_list->get_style_context()->add_class("title");
            lbl_list->set_halign(Gtk::ALIGN_START);
            main_box.pack_start(*lbl_list);

            auto networks = get_wifi_networks();
            
            auto action_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 4);
            auto btn_action = Gtk::make_managed<Gtk::Button>("Conectar");
            auto entry_password = Gtk::make_managed<Gtk::Entry>();
            auto btn_submit_pass = Gtk::make_managed<Gtk::Button>("✓");

            btn_action->get_style_context()->add_class("action-btn");
            action_box->pack_start(*btn_action, Gtk::PACK_SHRINK);
            action_box->pack_start(*entry_password, Gtk::PACK_EXPAND_WIDGET);
            action_box->pack_start(*btn_submit_pass, Gtk::PACK_SHRINK);

            struct WifiRow {
                Gtk::Button* button;
                std::string ssid;
            };
            auto rows = std::make_shared<std::vector<WifiRow>>();

            if (!networks.empty()) {
                for (const auto& net : networks) {
                    bool is_current = (net == current_connected);
                    auto net_btn = Gtk::make_managed<Gtk::Button>((is_current ? "󰄲  " : "  ") + net);
                    net_btn->get_style_context()->add_class("wifi-list-btn");
                    
                    // Alinha o texto interno do botão 100% para a esquerda (0.0) de forma segura
                    net_btn->set_alignment(0.0, 0.5); 
                    
                    main_box.pack_start(*net_btn);
                    rows->push_back({net_btn, net});

                    // Evento de clique na rede
                    net_btn->signal_clicked().connect([net_btn, net, rows, action_box, btn_action, entry_password, btn_submit_pass, is_current, &main_box]() {
                        for (const auto& row : *rows) {
                            row.button->get_style_context()->remove_class("selected");
                        }
                        net_btn->get_style_context()->add_class("selected");

                        entry_password->hide();
                        btn_submit_pass->hide();
                        btn_action->show();
                        entry_password->set_text("");

                        main_box.reorder_child(*action_box, main_box.get_children().size() - 1); 
                        
                        if (is_current) {
                            btn_action->set_label("Desconectar");
                            btn_action->get_style_context()->add_class("disconnect");
                        } else {
                            btn_action->set_label("Conectar");
                            btn_action->get_style_context()->remove_class("disconnect");
                        }
                        action_box->show_all();
                        entry_password->hide(); 
                        btn_submit_pass->hide();
                    });
                }
                
                main_box.pack_start(*action_box);
                action_box->hide();

                // Clique do botão Conectar / Desconectar
                btn_action->signal_clicked().connect([btn_action, entry_password, btn_submit_pass, rows]() {
                    std::string selected_ssid = "";
                    for (const auto& row : *rows) {
                        if (row.button->get_style_context()->has_class("selected")) {
                            selected_ssid = row.ssid;
                            break;
                        }
                    }
                    if (selected_ssid.empty()) return;

                    if (btn_action->get_label() == "Desconectar") {
                        std::string cmd = "/usr/bin/nmcli device disconnect wlan0 &"; 
                        std::system(cmd.c_str());
                        Gtk::Main::quit();
                    } else {
                        if (network_needs_password(selected_ssid)) {
                            btn_action->hide();
                            entry_password->set_placeholder_text("Senha para " + selected_ssid);
                            entry_password->set_visibility(false); 
                            entry_password->show();
                            btn_submit_pass->show();
                        } else {
                            std::string cmd = "/usr/bin/nmcli device wifi connect '" + selected_ssid + "' &";
                            std::system(cmd.c_str());
                            Gtk::Main::quit();
                        }
                    }
                });

                // Ação de confirmar a senha (Botão ✓)
                auto connect_with_pass = [entry_password, rows]() {
                    std::string selected_ssid = "";
                    for (const auto& row : *rows) {
                        if (row.button->get_style_context()->has_class("selected")) {
                            selected_ssid = row.ssid;
                            break;
                        }
                    }
                    std::string password = entry_password->get_text();
                    if (!selected_ssid.empty() && !password.empty()) {
                        std::string cmd = "/usr/bin/nmcli device wifi connect '" + selected_ssid + "' password '" + password + "' &";
                        std::system(cmd.c_str());
                        Gtk::Main::quit();
                    }
                };

                btn_submit_pass->signal_clicked().connect(connect_with_pass);
                entry_password->signal_activate().connect(connect_with_pass); 
                
            } else {
                auto lbl_none = Gtk::make_managed<Gtk::Label>("Nenhuma rede encontrada...");
                lbl_none->set_halign(Gtk::ALIGN_START);
                main_box.pack_start(*lbl_none);
            }
        }
    }

    // Fechar ao perder foco
    window.signal_focus_out_event().connect([&window](GdkEventFocus*) {
        Gtk::Main::quit();
        return false;
    });

    // Fechar com a tecla ESC
    window.signal_key_press_event().connect([&window](GdkEventKey* key_event) {
        if (key_event->keyval == GDK_KEY_Escape) {
            Gtk::Main::quit();
            return true;
        }
        return false;
    });

    window.add(main_box);
    window.show_all();

    return app->run(window);
}

// Compilar
// g++ NetworkWindow.cpp -o /home/lulu/.config/waybar/NetworkWindow $(pkg-config gtkmm-3.0 --cflags --libs) -lgtk-layer-shell