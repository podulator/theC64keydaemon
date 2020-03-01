#include <linux/input.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <alsa/asoundlib.h>

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)

struct HotKey {
	bool enabled;
	int keysym;
};

int getVolume() {
	int result = -1;
	long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";

    int sndFd = snd_mixer_open(&handle, 0);
    if (0 == sndFd) {
        int cardFd = snd_mixer_attach(handle, card);
        if (0 == cardFd) {
            snd_mixer_selem_register(handle, NULL, NULL);
            snd_mixer_load(handle);
            snd_mixer_selem_id_alloca(&sid);
            snd_mixer_selem_id_set_index(sid, 0);
            snd_mixer_selem_id_set_name(sid, selem_name);
            snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

            if (NULL != elem) {
                snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
                long unscaled = -1;
                snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &unscaled);
                result = (int) ((unscaled * 100) / max);
			}
		}
		snd_mixer_close(handle);
	}
	std::cout << "getVolume : " << result << std::endl;
	return result;
}

void setVolume(int val) {
	
	std::cout << "Setting volume to : " << val << std::endl;

    long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";

    int sndFd = snd_mixer_open(&handle, 0);
    if (0 == sndFd) {
        int cardFd = snd_mixer_attach(handle, card);
        if (0 == cardFd) {
            snd_mixer_selem_register(handle, NULL, NULL);
            snd_mixer_load(handle);

            snd_mixer_selem_id_alloca(&sid);
            snd_mixer_selem_id_set_index(sid, 0);
            snd_mixer_selem_id_set_name(sid, selem_name);
            snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

			if (NULL != elem) {
                snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
                snd_mixer_selem_set_playback_volume_all(elem, val * max / 100);
			}
		} 
		snd_mixer_close(handle);
		std::cout << "setVolume successful" << std::endl;
	}
}

void KeyboardMonitor (void) {
	int FileDevice;
	int ReadDevice;
	int Index;

	struct input_event InputEvent[64];
	int version;
	unsigned short id[4];
	unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
	
	// ctrl + cbm key
	const int comboSize = 2;
	HotKey combo[2] = {
		HotKey { enabled: false, keysym: 125}, 
		HotKey { enabled: false, keysym: 29 }
	}; 
	HotKey volumeUp = { enabled: false, keysym: 59 }; // f1
	HotKey volumeDown = { enabled: false, keysym: 61 }; // f3
		
	//----- OPEN THE INPUT DEVICE -----
	if ((FileDevice = open("/dev/input/event1", O_RDONLY)) < 0)	{
		perror("KeyboardMonitor can't open input device");
		close(FileDevice);
		return;
	}

	//----- GET DEVICE VERSION -----
	if (ioctl(FileDevice, EVIOCGVERSION, &version)) {
		perror("KeyboardMonitor can't get version");
		close(FileDevice);
		return;
	}
	printf("Input driver version is %d.%d.%d\n", version >> 16, (version >> 8) & 0xff, version & 0xff);

	//----- GET DEVICE INFO -----
	ioctl(FileDevice, EVIOCGID, id);
	printf("Input device ID: bus 0x%x vendor 0x%x product 0x%x version 0x%x\n", id[ID_BUS], id[ID_VENDOR], id[ID_PRODUCT], id[ID_VERSION]);
	
	memset(bit, 0, sizeof(bit));
	ioctl(FileDevice, EVIOCGBIT(0, EV_MAX), bit[0]);

	// read starting volume
	int desiredVolume = 80;
	int volume = getVolume();
	int escape = 0;
	while (volume != desiredVolume && escape++ < 10) {
		setVolume(desiredVolume);
		volume = getVolume();
		sleep(escape);
	}
	std::cout << "Volume : " << volume << std::endl;

	//----- READ KEYBOARD EVENTS -----
	while (1) {
		ReadDevice = read(FileDevice, InputEvent, sizeof(struct input_event) * 64);

		if (ReadDevice < (int) sizeof(struct input_event)) {
			perror("KeyboardMonitor error reading - keyboard lost?");
			close(FileDevice);
			return;
		} else {
			int numEv = ReadDevice / sizeof(struct input_event);
			std::cout << "Read " << numEv << "events" << std::endl;

			for (Index = 0; Index < numEv; Index++) {
				
				//	InputEvent[Index].time		timeval: 16 bytes (8 bytes for seconds, 8 bytes for microseconds)
				//	InputEvent[Index].type		See input-event-codes.h
				//	InputEvent[Index].code		See input-event-codes.h
				//	InputEvent[Index].value		01 for keypress, 00 for release, 02 for autorepeat
				
				if (InputEvent[Index].type == EV_KEY) {
					
					if (InputEvent[Index].value == 2) {
						// auto repeat
						std::cout << (int)(InputEvent[Index].code) << " Auto Repeat" << std::endl;
						// auto repeat is ok for combo only
						for (int x = 0; x < comboSize; x++) {
							if (combo[x].keysym == (int)(InputEvent[Index].code)) {
								combo[x].enabled = true;
								continue;
							}
						}
					} else if (InputEvent[Index].value == 1) {
						std::cout << (int)(InputEvent[Index].code) << " Key Down" << std::endl;
						// check the combos and discrete keys for key down
						for (int x = 0; x < comboSize; x++) {
							if (combo[x].keysym == (int)(InputEvent[Index].code)) {
								combo[x].enabled = true;
								continue;
							}
						}
						if (volumeUp.keysym == (int)(InputEvent[Index].code)) {
							volumeUp.enabled = true;
							continue;
						} else if (volumeDown.keysym == (int)(InputEvent[Index].code)) {
							volumeDown.enabled = true;
							continue;
						}

					} else if (InputEvent[Index].value == 0) {
						std::cout << (int)(InputEvent[Index].code) << " Key Up" << std::endl;
						// unset flags
						for (int x = 0; x < comboSize; x++) {
							if (combo[x].keysym == (int)(InputEvent[Index].code)) {
								combo[x].enabled = false;
								continue;
							}
						}
						if (volumeUp.keysym == (int)(InputEvent[Index].code)) {
							volumeUp.enabled = false;
							continue;
						} else if (volumeDown.keysym == (int)(InputEvent[Index].code)) {
							volumeDown.enabled = false;
							continue;
						}
					}
					
				}
			}
			std::cout << "Batch handling finished, checking actions" << std::endl;
			if (combo[0].enabled && combo[1].enabled) {
				std::cout << "Potential actions" << std::endl;
				if (volumeUp.enabled) {
					std::cout << "Raising the volume" << std::endl;
					volume += 5;
					if (volume > 100) volume = 100;
					setVolume(volume);
				} else if (volumeDown.enabled) {
					std::cout << "Lowering the volume" << std::endl;
					volume -= 5;
					if (volume < 0) volume = 0;
					setVolume(volume);
				}
			}
		}
	}
}

int main(void) {
	KeyboardMonitor();
	return 0;
}
