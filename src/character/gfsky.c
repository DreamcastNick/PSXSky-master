/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "gfsky.h"

#include "../mem.h"
#include "../archive.h"
#include "../stage.h"
#include "../main.h"

#include "../stage/week7.h"

//GFSKY character structure
enum
{
	GFSKY_ArcMain_BopLeft0,
	GFSKY_ArcMain_BopLeft1,
	GFSKY_ArcMain_BopRight0,
	GFSKY_ArcMain_BopRight1,
	GFSKY_ArcMain_Cry,
	
	GFSKY_Arc_Max,
};

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main;
	IO_Data arc_ptr[GFSKY_Arc_Max];
	
	Gfx_Tex tex;
	u8 frame, tex_id;
	
	//Pico test
	u16 *pico_p;
} Char_GFSKY;

//GFSKY character definitions
static const CharFrame char_gfsky_frame[] = {
	{GFSKY_ArcMain_BopLeft0, {    0,   0,  128, 256}, { 39,  160}}, //0 bop left 1
	{GFSKY_ArcMain_BopLeft0, {  128,   0,  256, 256}, { 39,  160}}, //1 bop left 2
	{GFSKY_ArcMain_BopLeft1, {    0,   0,  128, 256}, { 39,  160}}, //2 bop left 3
	{GFSKY_ArcMain_BopLeft1, {  128,   0,  256, 256}, { 39,  160}}, //3 bop left 1
	
	{GFSKY_ArcMain_BopRight0, {   0,   0,  128, 256}, { 39,  160}}, //4 bop right 1
	{GFSKY_ArcMain_BopRight0, { 128,   0,  256, 256}, { 39,  160}}, //5 bop right 2
	{GFSKY_ArcMain_BopRight1, {   0,   0,  128,  256}, { 39, 160}}, //6 bop right 3
	{GFSKY_ArcMain_BopRight1, { 128,   0,  256,  256}, { 39, 160}}, //7 bop right 3
	
	{GFSKY_ArcMain_Cry, {   0,   0,  128, 256}, { 37,  160}}, //8 cry
	{GFSKY_ArcMain_Cry, { 128,   0,  256, 256}, { 37,  160}}, //9 cry
};

static const Animation char_gfsky_anim[CharAnim_Max] = {
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Left}},                           //CharAnim_Idle
	{1, (const u8[]){ 0,  0,  0,  1,  1,  1,  2,  2,  2,  3,  3,  3,  ASCR_BACK, 1}}, //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Left}},                           //CharAnim_LeftAlt
	{2, (const u8[]){8, 9, ASCR_REPEAT}},                                  //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Left}},                           //CharAnim_DownAlt
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Left}},                           //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Left}},                           //CharAnim_UpAlt
	{1, (const u8[]){ 4,  4,  4,  5,  5,  5,  6,  6,  6,  7,  7,  7, ASCR_BACK, 1}}, //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Left}},                           //CharAnim_RightAlt
};

//GFSKY character functions
void Char_GFSKY_SetFrame(void *user, u8 frame)
{
	Char_GFSKY *this = (Char_GFSKY*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_gfsky_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_GFSKY_Tick(Character *character)
{
	Char_GFSKY *this = (Char_GFSKY*)character;
	
	//Initialize Pico test
	if (stage.stage_id == StageId_7_3 && stage.back != NULL && this->pico_p == NULL)
		this->pico_p = ((Back_Week7*)stage.back)->pico_chart;
	
	if (this->pico_p != NULL)
	{
		if (stage.note_scroll >= 0)
		{
			//Scroll through Pico chart
			u16 substep = stage.note_scroll >> FIXED_SHIFT;
			while (substep >= ((*this->pico_p) & 0x7FFF))
			{
				//Play animation and bump speakers
				character->set_anim(character, ((*this->pico_p) & 0x8000) ? CharAnim_Right : CharAnim_Left);
				this->pico_p++;
			}
		}
	}
	else
	{
		if (stage.flag & STAGE_FLAG_JUST_STEP)
		{
			//Perform dance
			if ((stage.song_step % stage.gf_speed) == 0)
			{
				//Switch animation
				if (character->animatable.anim == CharAnim_Left)
					character->set_anim(character, CharAnim_Right);
				else
					character->set_anim(character, CharAnim_Left);
			}
		}
	}
	
	//Animate and draw
	Animatable_Animate(&character->animatable, (void*)this, Char_GFSKY_SetFrame);
	Character_Draw(character, &this->tex, &char_gfsky_frame[this->frame]);
}

void Char_GFSKY_SetAnim(Character *character, u8 anim)
{
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
}

void Char_GFSKY_Free(Character *character)
{
	Char_GFSKY *this = (Char_GFSKY*)character;
	
	//Free art
	Mem_Free(this->arc_main);
}

Character *Char_GFSky_New(fixed_t x, fixed_t y)
{
	//Allocate gfsky object
	Char_GFSKY *this = Mem_Alloc(sizeof(Char_GFSKY));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_GFSky_New] Failed to allocate gfsky object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_GFSKY_Tick;
	this->character.set_anim = Char_GFSKY_SetAnim;
	this->character.free = Char_GFSKY_Free;
	
	Animatable_Init(&this->character.animatable, char_gfsky_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = 0;
	
	this->character.health_i = 1;
	
	this->character.focus_x = FIXED_DEC(16,1);
	this->character.focus_y = FIXED_DEC(-50,1);
	this->character.focus_zoom = FIXED_DEC(13,10);
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\GFSKY.ARC;1");
	
	const char **pathp = (const char *[]){
		"bopleft0.tim",  //GFSKY_ArcMain_BopLeft0
		"bopleft1.tim",  //GFSKY_ArcMain_BopLeft1
		"bopright0.tim", //GFSKY_ArcMain_BopRight0
		"bopright1.tim", //GFSKY_ArcMain_BopRight1
		"cry.tim",      //GFSKY_ArcMain_Cry
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	//Initialize Pico test
	if (stage.stage_id == StageId_7_3 && stage.back != NULL)
		this->pico_p = ((Back_Week7*)stage.back)->pico_chart;
	else
		this->pico_p = NULL;
	
	return (Character*)this;
}
