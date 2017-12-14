/*****************************************************************************
 * DikuMUD (C) 1990, 1991 by:                                                *
 *   Sebastian Hammer, Michael Seifert, Hans Henrik Staefeldt, Tom Madsen,   *
 *   and Katja Nyboe.                                                        *
 *---------------------------------------------------------------------------*
 * MERC 2.1 (C) 1992, 1993 by:                                               *
 *   Michael Chastain, Michael Quan, and Mitchell Tse.                       *
 *---------------------------------------------------------------------------*
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998 by: Derek Snider.                    *
 *   Team: Thoric, Altrag, Blodkai, Narn, Haus, Scryn, Rennard, Swordbearer, *
 *         gorog, Grishnakh, Nivek, Tricops, and Fireblade.                  *
 *---------------------------------------------------------------------------*
 * SMAUG 1.7 FUSS by: Samson and others of the SMAUG community.              *
 *                    Their contributions are greatly appreciated.           *
 *---------------------------------------------------------------------------*
 * LoP (C) 2006 - 2012 by: the LoP team.                                     *
 *---------------------------------------------------------------------------*
 * Win32 port by Nick Gammon                                                 *
 *---------------------------------------------------------------------------*
 *                           Db.c mud header file                            *
 *****************************************************************************/

/* These are skill_lookup return values for common skills and spells. */
extern int gsn_style_evasive;
extern int gsn_style_defensive;
extern int gsn_style_standard;
extern int gsn_style_aggressive;
extern int gsn_style_berserk;

extern int gsn_taste;
extern int gsn_smell;
extern int gsn_detrap;
extern int gsn_trapset;
extern int gsn_backstab;
extern int gsn_circle;
extern int gsn_cook;
extern int gsn_dodge;
extern int gsn_duck;
extern int gsn_block;
extern int gsn_shieldblock;
extern int gsn_counter;
extern int gsn_hide;
extern int gsn_peek;
extern int gsn_pick_lock;
extern int gsn_scan;
extern int gsn_sneak;
extern int gsn_steal;
extern int gsn_gouge;
extern int gsn_track;
extern int gsn_search;
extern int gsn_dig;
extern int gsn_mount;
extern int gsn_bashdoor;
extern int gsn_berserk;

extern int gsn_disarm;
extern int gsn_enhanced_damage;
extern int gsn_kick;
extern int gsn_parry;
extern int gsn_rescue;
extern int gsn_feed;
extern int gsn_chop;
extern int gsn_makefire;
extern int gsn_bloodlet;

extern int gsn_aid;

/* used to do specific lookups */
extern int gsn_first_spell;
extern int gsn_first_skill;
extern int gsn_first_weapon;
extern int gsn_first_tongue;
extern int gsn_top_sn;

/* spells */
extern int gsn_blindness;
extern int gsn_charm_person;
extern int gsn_aqua_breath;
extern int gsn_curse;
extern int gsn_invis;
extern int gsn_poison;
extern int gsn_sleep;

/* newer attack skills */
extern int gsn_punch;
extern int gsn_bash;
extern int gsn_stun;
extern int gsn_bite;
extern int gsn_claw;
extern int gsn_tail;

extern int gsn_poison_weapon;
extern int gsn_scribe;
extern int gsn_brew;
extern int gsn_imbue;
extern int gsn_carve;
extern int gsn_concoct;
extern int gsn_mix;
extern int gsn_climb;

extern int gsn_barehanded;
extern int gsn_pugilism;
extern int gsn_long_blades;
extern int gsn_short_blades;
extern int gsn_flexible_arms;
extern int gsn_talonous_arms;
extern int gsn_bludgeons;
extern int gsn_missile_weapons;

extern int gsn_grip;
extern int gsn_slice;

extern int gsn_tumble;

/* Language gsns. -- Altrag */
extern int gsn_common;

/* This is to tell if act uses uppercasestring or not --Shaddai */
extern bool DONT_UPPER;
extern bool MOBtrigger;
extern GROUP_DATA *first_group, *last_group;
extern int weath_unit;
extern int rand_factor;
extern int climate_factor;
extern int neigh_factor;
extern int max_vector;
extern CHAR_DATA *first_char, *last_char;
extern OBJ_DATA *first_object, *last_object;
extern OBJ_DATA *first_corpse, *last_corpse;
extern AREA_DATA *first_area, *last_area;
extern AREA_DATA *first_build, *last_build;
extern AREA_DATA *first_asort, *last_asort;
extern AREA_DATA *first_bsort, *last_bsort;
extern AREA_DATA *first_area_name, *last_area_name;
extern SHOP_DATA *first_shop, *last_shop;
extern REPAIR_DATA *first_repair, *last_repair;
extern time_t last_restore_all_time;
extern FILE *fpArea;
extern char strArea[MIL];
extern int numobjsloaded;
extern int nummobsloaded;
extern int physicalobjects;
extern int num_corpses;
extern SYSTEM_DATA sysdata;
extern EXTRACT_CHAR_DATA  *extracted_char_queue;
extern int top_reset;
extern int top_area;
extern int top_affect;
extern int top_ed;
extern int top_exit;
extern int cur_qchars;
extern bool fBootDb;
extern bool unfoldload;
extern bool unfoldbadload;
extern MOB_INDEX_DATA *mob_index_hash[MKH];
extern OBJ_INDEX_DATA *obj_index_hash[MKH];
extern ROOM_INDEX_DATA *room_index_hash[MKH];
CHAR_DATA *create_mobile( MOB_INDEX_DATA *pMobIndex );
OBJ_DATA *create_object( OBJ_INDEX_DATA *pObjIndex, int level );
MOB_INDEX_DATA *get_mob_index( int vnum );
MOB_INDEX_DATA *make_mobile( int vnum, int cvnum, char *name );
OBJ_INDEX_DATA *make_object( int vnum, int cvnum, char *name );
OBJ_INDEX_DATA *get_obj_index( int vnum );
OBJ_INDEX_DATA *new_object( int vnum );
ROOM_INDEX_DATA *get_room_index( int vnum );
ROOM_INDEX_DATA *get_new_room_index( int avnum, int vnum );
ROOM_INDEX_DATA *new_room( int vnum );
ROOM_INDEX_DATA *make_room( int vnum, AREA_DATA *area );
EXIT_DATA *make_exit( ROOM_INDEX_DATA *pRoomIndex, ROOM_INDEX_DATA *to_room, short door );
const char *format( const char *fmt, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
char *get_extra_descr( const char *name, EXTRA_DESCR_DATA *ed );
char *str_dup( char const *str );
char *fread_flagstring( FILE *fp );
char *fread_string( FILE *fp );
char *fread_line( FILE *fp );
char *fread_word( FILE *fp );
char *show_tilde( const char *str );
char *capitalize( const char *str );
char *strlower( const char *str );
char *strupper( const char *str );
char *aoran( const char *str );
char fread_letter( FILE *fp );
void load_objects( AREA_DATA *tarea, FILE *fp );
void add_letter( char *string, char letter );
void fread_to_eol( FILE *fp );
void clear_char( CHAR_DATA *ch );
void free_char( CHAR_DATA *ch );
void shutdown_mud( const char *reason );
void boot_db( bool fCopyOver );
void add_char( CHAR_DATA *ch );
void fix_exits( void );
void randomize_exits( ROOM_INDEX_DATA *room, short maxdir );
void load_area_file( AREA_DATA *tarea, char *filename );
void load_buildlist( void );
void sort_area_by_name( AREA_DATA *pArea );
void sort_area( AREA_DATA *pArea, bool proto );
void tail_chain( void );
void init_area_weather( void );
void load_weatherdata( void );
void save_weatherdata( void );
void area_update( void );
void init_mm( void );
void smash_tilde( char *str );
void hide_tilde( char *str );
void append_file( CHAR_DATA *ch, const char *file, const char *str );
void append_to_file( const char *file, const char *str );
void bug( const char *str, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
void boot_log( const char *str, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
void show_file( CHAR_DATA *ch, const char *filename );
void make_wizlist( void );
void mprog_read_programs( FILE *fp, MOB_INDEX_DATA *mob );
void oprog_read_programs( FILE *fp, OBJ_INDEX_DATA *obj );
void rprog_read_programs( FILE *fp, ROOM_INDEX_DATA *room );
void delete_room( ROOM_INDEX_DATA *room );
void delete_obj( OBJ_INDEX_DATA *obj );
void delete_mob( MOB_INDEX_DATA *mob );
size_t mudstrlcat( char *dst, const char *src, size_t siz );
size_t newmudstrlcpy( char *dst, const char *src, size_t siz, const char *filename, int line );
#define mudstrlcpy( dst, src, siz )  newmudstrlcpy( (dst), (src), (siz), __FILE__, __LINE__ )
time_t fread_time( FILE *fp );
double fread_double( FILE *fp );
unsigned int fread_un_number( FILE *fp );
int interpolate( int level, int value_00, int value_32 );
int number_range( int from, int to );
int number_percent( void );
int number_door( void );
int number_bits( int width );
int number_mm( void );
int dice( int number, int size );
int fread_number( FILE *fp );
bool remove_line_from_file( const char *filename, int dline );
bool str_cmp( const char *astr, const char *bstr );
bool str_prefix( const char *astr, const char *bstr );
bool str_infix( const char *astr, const char *bstr );
bool str_suffix( const char *astr, const char *bstr );
