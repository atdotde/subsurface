/* main.c */
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "qt-gui.h"
#include "subsurfacestartup.h"
#include "subsurface-core/color.h"
#include "qthelper.h"
#include "helpers.h"

#include <QStringList>
#include <QApplication>
#include <QLoggingCategory>
#include <git2.h>

#include "dive.h"
#include "planner.h"
#include "units.h"
#include "subsurfacestartup.h"
#include "qthelper.h"
#include <QDebug>
#include "qt-mobile/qmlmanager.h"

#define DEBUG  1

// testing the dive plan algorithm
extern bool plan(struct diveplan *diveplan, char **cached_datap, bool is_planner, bool show_disclaimer);

extern pressure_t first_ceiling_pressure;

finalPlan finalplan;

QTranslator *qtTranslator, *ssrfTranslator;

void setupPlan(struct diveplan *dp)
{
	dp->salinity = 10300;
	dp->surface_pressure = 1013;
	dp->gfhigh = 100;
	dp->gflow = 100;
	dp->bottomsac = 0;
	dp->decosac = 0;

	struct gasmix bottomgas = { {150}, {450} };
	struct gasmix ean36 = { {360}, {0} };
	struct gasmix oxygen = { {1000}, {0} };
	pressure_t po2 = { 1600 };
	displayed_dive.cylinder[0].gasmix = bottomgas;
	displayed_dive.cylinder[1].gasmix = ean36;
	displayed_dive.cylinder[2].gasmix = oxygen;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	int droptime = M_OR_FT(79, 260) * 60 / M_OR_FT(23, 75);
	plan_add_segment(dp, droptime, M_OR_FT(79, 260), bottomgas, 0, 1);
	plan_add_segment(dp, 30*60 - droptime, M_OR_FT(79, 260), bottomgas, 0, 1);
	plan_add_segment(dp, 0, gas_mod(&ean36, po2, &displayed_dive, M_OR_FT(3,10)).mm, ean36, 0, 1);
	plan_add_segment(dp, 0, gas_mod(&oxygen, po2, &displayed_dive, M_OR_FT(3,10)).mm, oxygen, 0, 1);
}

int main(int argc, char **argv)
{
	int i;
	bool no_filenames = true;
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));
	QApplication *application = new QApplication(argc, argv);
	(void)application;
	QStringList files;
	QStringList importedFiles;
	QStringList arguments = QCoreApplication::arguments();
	qDebug() << "Hello";

	bool dedicated_console = arguments.length() > 1 &&
				 (arguments.at(1) == QString("--win32console"));
	subsurface_console_init(dedicated_console);

	for (i = 1; i < arguments.length(); i++) {
		QString a = arguments.at(i);
		if (a.at(0) == '-') {
			parse_argument(a.toLocal8Bit().data());
			continue;
		}
		if (imported) {
			importedFiles.push_back(a);
		} else {
			no_filenames = false;
			files.push_back(a);
		}
	}
#if !LIBGIT2_VER_MAJOR && LIBGIT2_VER_MINOR < 22
	git_threads_init();
#else
	git_libgit2_init();
#endif
	setup_system_prefs();
	if (uiLanguage(0).contains("-US"))
		default_prefs.units = IMPERIAL_units;
	prefs = default_prefs;
	fill_profile_color();
	parse_xml_init();
	taglist_init_global();
	init_ui();
	loadPreferences();
	prefs.animation_speed = 0;

	/* always show the divecomputer reported ceiling in red */
	prefs.dcceiling = 1;
	prefs.redceiling = 1;

	init_proxy();
//	if (no_filenames) {
//		if (prefs.default_file_behavior == LOCAL_DEFAULT_FILE) {
//			QString defaultFile(prefs.default_filename);
//			if (!defaultFile.isEmpty())
//				files.push_back(QString(prefs.default_filename));
//		} else if (prefs.default_file_behavior == CLOUD_DEFAULT_FILE) {
//			QString cloudURL;
//			if (getCloudURL(cloudURL) == 0)
//				files.push_back(cloudURL);
//		}
//	}

	char *cache = NULL;

	struct diveplan testPlan = { 0 };
	setupPlan(&testPlan);

	plan(&testPlan, &cache, 1, 0);

	qDebug() << displayed_dive.notes;
	finalplan.setPlan(displayed_dive.notes);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive);
#endif



	if (!quit)
		run_ui();
	exit_ui();
	taglist_free(g_tag_list);
	parse_xml_exit();
	subsurface_console_exit();
	free_prefs();
	return 0;
}
