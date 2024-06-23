#pragma once

//#define DEBUG
#define INFO
//#define NESDEBUG

#include <cstring>
#include <cstdint>
#include <vector>
#include "inih/cpp/INIReader.h"
#include "nes/BUS.h"

#define TBL_WIDTH 40  //32/40/54 (DIV 2 ONLY!)
#define MAX_SPRITES 256

const int16_t SCREEN_SIZE = TBL_WIDTH*8;
const int16_t TBL_SIZE    = TBL_WIDTH*30; //30 = 240/8
const int16_t HALF_TBL_WIDTH = TBL_WIDTH>>1;
const int16_t COURUSEL_START = HALF_TBL_WIDTH - 5;
const uint8_t maxSaveState = SCREEN_SIZE<320?2:3;

class Shell {
	const char * table_sprites = "usr\\share\\clover-ui\\resources\\sprites\\hvc.ppu"; 
	const char * bgm_boot_snd = "usr\\share\\clover-ui\\resources\\sounds\\bgm_boot.wav";
	const char * bgm_home_snd = "usr\\share\\clover-ui\\resources\\sounds\\bgm_home.wav";
	const char * se_sys_cursor_snd = "usr\\share\\clover-ui\\resources\\sounds\\se_sys_cursor.wav";   
	const char * se_sys_click_game_snd = "usr\\share\\clover-ui\\resources\\sounds\\se_sys_click_game.wav";
	const char * se_sys_click_snd = "usr\\share\\clover-ui\\resources\\sounds\\se_sys_click.wav";
	const char * se_sys_cancel_snd = "usr\\share\\clover-ui\\resources\\sounds\\se_sys_cancel.wav";
	const char * se_sys_smoke_snd = "usr\\share\\clover-ui\\resources\\sounds\\se_sys_smoke.wav";
	const char * game_dir = "usr\\share\\games\\nes\\kachikachi";

	private:
		enum selectors {empty, gamelist, menu, saves, display, options, about, manuals, playgame, preparegame};
		uint32_t counter = 0;
		

		uint8_t lastkbState;
		uint8_t tblName[TBL_SIZE];
		uint8_t	tblAttribute[TBL_SIZE];
		BUS NES;
		CARTRIDGE * CART;
		
		int16_t * audiosample = new int16_t[2048];
		int16_t * tempsample = new int16_t[2048];
		

		struct Settings {
			uint8_t disaplayScale = 1;
			uint8_t volume = 10;
			uint8_t brightness = 10; //Platform specific
			bool play_home_music = true;
			bool turbo_a = false;
			bool turbo_b = false;
		} setting;

		struct {
			uint32_t color[4];
		} pallete[32];

		struct {
			struct {
				uint16_t pattern[8];
			} sprite[0xFF];
		} sprites[2];

		struct BmpPixel {
		    uint8_t B, G, R;
		};
		struct Image{
			uint8_t width, height, crop_width;
			int16_t x,y;
			std::vector<BmpPixel> rawData;
		};

		struct sObjectAttributeEntry {
			int16_t y;			// Y position of sprite
			uint8_t id;			// ID of tile from pattern memory
			uint8_t attribute;	// Flags define how sprite should be rendered
			int16_t x;			// X position of sprite
			uint8_t opacity;
			Image * image;
		} OAM[MAX_SPRITES];


		struct WavHeader {
		    uint32_t chunkID;
		    uint32_t chunkSize;
		    uint32_t format;
		    uint32_t subchunk1ID;
		    uint32_t subchunk1Size;
		    uint16_t audioFormat;
		    uint16_t numChannels;
		    uint32_t sampleRate;
		    uint32_t byteRate;
		    uint16_t blockAlign;
		    uint16_t bitsPerSample;
		    uint32_t subchunk2ID;
		    uint32_t subchunk2Size;
		};

		struct WavFile {
			uint32_t offset;
			FILE * file;
			WavHeader header;
		};
		WavFile * bgm_boot;
		WavFile * bgm_home;
		WavFile * se_sys_cursor;
		WavFile * se_sys_click_game;
		WavFile * se_sys_click;
		WavFile * se_sys_cancel;
		WavFile * se_sys_smoke;

		struct BmpHeader {
		    uint32_t sizeOfBitmapFile;
		    uint32_t reservedBytes;
		    uint32_t pixelDataOffset;
		};

		struct BmpInfoHeader {
		    uint32_t sizeOfThisHeader;
		    int32_t width;
		    int32_t height;
		    uint16_t numberOfColorPlanes;
		    uint16_t colorDepth;
		    uint32_t compressionMethod;
		    uint32_t rawBitmapDataSize;
		    int32_t horizontalResolution;
		    int32_t verticalResolution;
		    uint32_t colorTableEntries;
		    uint32_t importantColors;
		};

		struct SavePoint {
			uint8_t id;
			uint16_t time_shtamp;
			Image * image;
			State * state;
		};

		struct card{
			uint8_t id;
			int16_t x_pos;
			int16_t y_pos;
			bool select;
			bool * pSelect;
			std::string path;
			std::string exec;
			std::string name;
			std::string date;
			std::string copyright;
			std::string SortRawPublisher;
			std::string SortRawTitle;
			std::string text;
			uint8_t players;
			uint8_t simultaneous;
			Image * image;
			SavePoint * savePointList[4];
		};
		struct navcard{
			uint8_t id;
			int16_t x_pos;
			int16_t y_pos;
			bool * select;
			Image * image; 
		};

		std::vector<card*> courusel;
		std::vector<navcard*> navigate;
		
		int8_t fadeProcents = 0;
		int8_t courusel_direction = 0;
		int8_t select_direction = 0;
		int16_t stable_position = COURUSEL_START;
		int16_t select_to = stable_position;
		int16_t navigate_offset = 0;
		uint16_t max_length_navbar = 2;
		uint8_t select_menu_position = 0;
		int16_t select_to_menu_position = select_menu_position;
		int8_t select_save_state = 0;
		uint16_t temp_save_state_y = 0;
		uint16_t temp_save_state_x = 0;
		SavePoint * temp_save_state = NULL; 
		int16_t x_offset_save_state = 0;
		int16_t stable_position_save_state = 0;

		int16_t stable_position_syspend_panel_selector = 6;
		int16_t select_to_syspend_panel_selector = stable_position_syspend_panel_selector;
		bool no_select_state = false;

		uint8_t cursor = 0;
		int8_t displaySettingSelector = setting.disaplayScale;
		int8_t optionSettingSelector = 0;
		int16_t x_offset = stable_position;
		uint16_t max_length;
		uint8_t  controller;
		uint32_t * FrameBuffer;
		selectors currentSelect = gamelist;
		uint32_t avrColor(uint8_t a, uint8_t b, uint32_t c1, uint32_t c2);
		void PutPixel(int16_t px,int16_t py, uint32_t color);
		uint32_t GetPixel(int16_t px,int16_t py);
		
		bool deleteSavePoint(SavePoint * savepoint);
		void SaveSettings();
		void LoadSettings();
		bool loadWav(WavFile * wavFile, const char* fname);
		void SaveSaves();
		void LoadSaves(SavePoint * savePointList[4], uint8_t id, const char* fname);
		void PlayWavClock(WavFile * file, bool loop = false);
		void PlayWav(WavFile * file);
		void drawTopBar();
		void drawMenu();
		void drawTopBarIcons();
		void drawDownBar();
		void drawSavesPanel();
		void drawTempSaveState(uint16_t counter);
		void drawTitleBar(uint8_t x, uint8_t y, uint8_t l);
		void drawCourusel(uint8_t y, uint8_t counter);
		void drawNavigate(uint8_t counter);
		void drawSelector(selectors mode);
		void drawHintBar(selectors mode);
		void drawPanel();
		void drawDisplayContent(uint8_t counter);
		void drawOptionsContent(uint8_t counter);
		void drawAboutContent(uint8_t counter);
		void drawManualsContent(uint8_t counter);
		void renderFrameBuffer();
		void drawSaveState(uint8_t ndx, uint8_t xt, uint8_t yt);
		void drawSprite(uint8_t id, int16_t x, int16_t y, uint8_t attribute, uint8_t leng = 1, uint8_t width = 0, uint8_t opacity = 100);
		void loadBitmap(const char* fname, Image * image);
		void drawDisplayPanel(uint8_t counter);
		void drawOptionsPanel(uint8_t counter);
		void drawAboutPanel(uint8_t counter);
		void drawManualsPanel(uint8_t counter);
		uint8_t swapBit(uint8_t b);
		void MixArray(int16_t * a1, int16_t * a2, uint32_t l) {
			for (uint32_t i = 0; i<l; i++) a1[i] = ((a1[i]+a2[i]) * (float)setting.volume/10);
		}
	public: 
		const unsigned SoundSamplesPerSec = 44100;
		Shell();
		~Shell(){}
		bool LoadData();
		void setJoyState(uint8_t kb);
		void SetFrameBuffer(uint32_t * fb) { FrameBuffer = fb; }
		void Update(double FPS);
		int16_t * getAudioSample() {return audiosample;}
		bool frame_complete = false;
		uint32_t audioBufferCount = 0;
};