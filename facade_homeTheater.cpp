#include <iostream>
using namespace std;

// Subsystem 1
class TV {
public:
    void turnOn() {
        cout << "TV dinyalakan" << endl;
    }

    void turnOff() {
        cout << "TV dimatikan" << endl;
    }
};

// Subsystem 2
class SoundSystem {
public:
    void turnOn() {
        cout << "Sound system dinyalakan" << endl;
    }

    void setVolume(int volume) {
        cout << "Volume sound system diatur ke " << volume << endl;
    }

    void turnOff() {
        cout << "Sound system dimatikan" << endl;
    }
};

// Subsystem 3
class DVDPlayer {
public:
    void turnOn() {
        cout << "DVD player dinyalakan" << endl;
    }

    void playMovie(const string& movie) {
        cout << "Memutar film: " << movie << endl;
    }

    void turnOff() {
        cout << "DVD player dimatikan" << endl;
    }
};

// Facade
class HomeTheaterFacade {
private:
    TV tv;
    SoundSystem soundSystem;
    DVDPlayer dvdPlayer;

public:
    void watchMovie(const string& movie) {
        cout << "Menyiapkan home theater..." << endl;

        tv.turnOn();
        soundSystem.turnOn();
        soundSystem.setVolume(80);
        dvdPlayer.turnOn();
        dvdPlayer.playMovie(movie);

        cout << "Film siap ditonton" << endl;
    }

    void endMovie() {
        cout << "Mematikan home theater..." << endl;

        dvdPlayer.turnOff();
        soundSystem.turnOff();
        tv.turnOff();

        cout << "Home theater sudah dimatikan" << endl;
    }
};

// Client
int main() {
    HomeTheaterFacade homeTheater;

    homeTheater.watchMovie("Hail Mary");

    cout << endl;

    homeTheater.endMovie();

    return 0;
}