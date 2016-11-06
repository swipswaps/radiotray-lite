#include "tray.hpp"

namespace radiotray {

RadioTrayLite::BookmarksWalker::BookmarksWalker(RadioTrayLite& radiotray, Gtk::Menu* menu)
    : radiotray(radiotray)
{
    menus.push(menu);
}

bool
RadioTrayLite::BookmarksWalker::for_each(pugi::xml_node& node)
{
    auto name = node.name();
    auto attr_name = node.attribute("name");
    auto attr_url = node.attribute("url");

    if (attr_name.empty() or strcasecmp(attr_name.as_string(), "root") == 0) {
        return true; // continue traversal
    }

    auto is_group = strcasecmp(name, "group") == 0;
    auto is_bookmark = strcasecmp(name, "bookmark") == 0;

    if (is_group) {
        while (menus.size() >= static_cast<size_t>(depth())) {
            menus.pop();
        }
        auto group_name = attr_name.as_string();
        auto menu_item = Gtk::manage(new Gtk::MenuItem(group_name));
        auto submenu = Gtk::manage(new Gtk::Menu());
        menus.top()->append(*menu_item);
        menu_item->set_submenu(*submenu);
        menus.push(submenu);
        level = depth();
        std::cout << "Group: " << group_name << ", depth: " << depth() << std::endl;
    } else if (is_bookmark and (!attr_url.empty())) {
        auto station_name = attr_name.as_string();
        auto station_group_name = node.parent().attribute("name").as_string();
        auto station_url = attr_url.as_string();
        auto sub_item = Gtk::manage(new Gtk::MenuItem(station_name));
        sub_item->signal_activate().connect(sigc::bind<Glib::ustring, Glib::ustring, Glib::ustring>(
                sigc::mem_fun(radiotray, &RadioTrayLite::on_station_button), station_group_name, station_name, station_url));
        menus.top()->append(*sub_item);
        std::cout << "Bookmark depth: " << depth() << ", level: " << level << ", #menus: " << menus.size() <<  ", station: " << station_name << ", group: " << station_group_name << std::endl;
    }

    std::cout.flush();

    return true; // continue traversal
}

RadioTrayLite::RadioTrayLite(int argc, char** argv)
{
    app = Gtk::Application::create(argc, argv, "github.com.thekvs.radiotray-lite");
    menu = std::make_shared<Gtk::Menu>();

    player = std::make_shared<Player>();
    auto ok = player->init(argc, argv);
    if (!ok) {
        // TODO
    }

    em = std::make_shared<EventManager>();
    em->state_changed.connect(sigc::mem_fun(*this, &RadioTrayLite::on_station_changed_signal));

    player->em = em;

    indicator = app_indicator_new_with_path("Radio Tray Lite", kAppIndicatorIconOff,
        APP_INDICATOR_CATEGORY_APPLICATION_STATUS, kImagePath);
    app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_attention_icon(indicator, kAppIndicatorIconOn);
    app_indicator_set_menu(indicator, menu->gobj());

    search_for_bookmarks_file();
}

RadioTrayLite::~RadioTrayLite()
{
    clear_menu();

    if (indicator != nullptr) {
        g_object_unref(G_OBJECT(indicator));
    }
}

void
RadioTrayLite::run()
{
    build_menu();

    // FIXME: WHY THIS DOESN'T RUN IN A LOOP?
    // app->run();

    gtk_main();
}

void
RadioTrayLite::on_quit_button()
{
    std::cout << "'Quit' button was pressed." << std::endl;

    player->quit();
    gtk_main_quit();
}

void
RadioTrayLite::on_about_button()
{
    std::cout << "'About' button was pressed." << std::endl;
}

void
RadioTrayLite::on_station_button(Glib::ustring group_name, Glib::ustring station_name, Glib::ustring station_url)
{
#if 0
    auto mk_name = [](Glib::ustring name, bool turn_on)// -> Glib::ustring
    {
        std::stringstream ss;
        if (turn_on) {
            ss << "Turn On \"" << name << "\"";
        } else {
            ss << "Turn Off \"" << name << "\"";
        }

        return Glib::ustring(ss.str());
    };
#endif

    current_station = station_name;

#if 0
    if (current_station_menu_entry == nullptr) {
        auto separator_item = Gtk::manage(new Gtk::SeparatorMenuItem());
        menu->prepend(*separator_item);
        current_station_menu_entry = Gtk::manage(new Gtk::MenuItem(mk_name(current_station, false)));
        menu->prepend(*current_station_menu_entry);
        menu->show_all();
    } else {
        current_station_menu_entry->set_label(mk_name(current_station, false));
    }
#endif

    player->play(station_url, current_station);

    std::cout << "'" << station_url << "'" << "(group: " << group_name << ")" << " button was pressed." << std::endl;
}

void
RadioTrayLite::on_reload_button()
{
    std::cout << "'Reload'" << " button was pressed" << std::endl;
    rebuild_menu();
}

void
RadioTrayLite::build_menu()
{
    auto bookmarks_parsed = parse_bookmarks_file();
    if (bookmarks_parsed) {
        BookmarksWalker walker(*this, &(*menu));
        bookmarks_doc.traverse(walker);
    } else {
        // TODO: notify about parsing errors
    }

    Glib::ustring name;

    auto separator_item = Gtk::manage(new Gtk::SeparatorMenuItem());
    menu->append(*separator_item);

    name = mk_name("Reload Bookmarks");
    auto menu_item = Gtk::manage(new Gtk::MenuItem(name));
    menu_item->signal_activate().connect(sigc::mem_fun(*this, &RadioTrayLite::on_reload_button));
    menu->append(*menu_item);

    name = "About";
    menu_item = Gtk::manage(new Gtk::MenuItem(name));
    menu_item->signal_activate().connect(sigc::mem_fun(*this, &RadioTrayLite::on_about_button));
    menu->append(*menu_item);

    name = "Quit";
    menu_item = Gtk::manage(new Gtk::MenuItem(name));
    menu_item->signal_activate().connect(sigc::mem_fun(*this, &RadioTrayLite::on_quit_button));
    menu->append(*menu_item);

    if (not current_station.empty()) {
        auto separator_item = Gtk::manage(new Gtk::SeparatorMenuItem());
        menu->prepend(*separator_item);
        current_station_menu_entry = Gtk::manage(new Gtk::MenuItem(current_station));
        menu->prepend(*current_station_menu_entry);
    }

    menu->show_all();
}

void
RadioTrayLite::rebuild_menu()
{
    counter++;

    clear_menu();
    build_menu();
}

void
RadioTrayLite::clear_menu()
{
    for (auto &e : menu->get_children()) {
        menu->remove(*e);
        delete e;
    }
}

bool
RadioTrayLite::parse_bookmarks_file()
{
    bool status = false;

    if (not bookmarks_file.empty()) {
        pugi::xml_parse_result result = bookmarks_doc.load_file(bookmarks_file.c_str());
        if (!result) {
            std::cerr << "XML parser failed: " << result.description() << std::endl;
        } else {
            status = true;
        }
    } else {
        std::cerr << "Bookmarks file not specified!" << std::endl;
    }

    return status;
}

void
RadioTrayLite::search_for_bookmarks_file()
{
    std::vector<std::string> paths;

    auto home = getenv("HOME");
    if (home != nullptr) {
        auto dir = std::string(home) + "/.config/" + kAppDirName + "/" + kBookmarksFileName;
        paths.push_back(dir);
        dir = std::string(home) + "/.local/share/" + kAppDirName + "/" + kBookmarksFileName;
        paths.push_back(dir);
    }

    // default bookmarks file
    paths.push_back(std::string("/usr/share/") + kAppDirName + "/" + kBookmarksFileName);

    auto file_exists = [](const std::string& fname) -> bool
    {
        struct stat st;
        auto rc = stat(fname.c_str(), &st);
        if (rc == 0 and S_ISREG(st.st_mode)) {
            return true;
        }
        return false;
    };

    auto result = std::find_if(std::begin(paths), std::end(paths), file_exists);
    if (result != std::end(paths)) {
        bookmarks_file = *result;
    }
}

void
RadioTrayLite::on_station_changed_signal(Glib::ustring station, StationState state)
{
    if (state == em->state and station == current_station) {
        return;
    }

    auto mk_menu_entry = [](Glib::ustring name, bool turn_on)
    {
        std::stringstream ss;
        if (turn_on) {
            ss << "Turn On \"" << name << "\"";
        } else {
            ss << "Turn Off \"" << name << "\"";
        }

        return Glib::ustring(ss.str());
    };

    auto turn_on = (state == StationState::PLAYING ? false : true);

    if (current_station_menu_entry == nullptr) {
        auto separator_item = Gtk::manage(new Gtk::SeparatorMenuItem());
        menu->prepend(*separator_item);
        current_station_menu_entry = Gtk::manage(new Gtk::MenuItem(mk_menu_entry(current_station, turn_on)));
        menu->prepend(*current_station_menu_entry);
    } else {
        current_station_menu_entry->set_label(mk_menu_entry(current_station, turn_on));
    }

    menu->show_all();

    if (state == StationState::PLAYING) {
        app_indicator_set_icon(indicator, kAppIndicatorIconOn);
    } else {
        app_indicator_set_icon(indicator, kAppIndicatorIconOff);
    }
}

void
RadioTrayLite::on_music_info_changed_signal(Glib::ustring station, Glib::ustring info)
{
}

Glib::ustring
RadioTrayLite::mk_name(Glib::ustring base_name)
{
    Glib::ustring result;
    char c[64];

    snprintf(c, sizeof(c), "%i", counter);
    result = base_name + " " + c;

    return result;
}
} // namespace radiotray
