///
/// @file main.cpp
/// @brief
///
/// @author Daniel Dolejška <xdolej08@stud.fit.vutbr.cz>
///
#include <functional>
#include "simlib/simlib.h"
#include "InputParser.h"

#define SECOND	1
#define MINUTE	(SECOND * 60)
#define HOUR	(MINUTE * 60)
#define WORKDAY	(HOUR * 14)

#define FREETIME (30 * MINUTE) /// PRES
#define WORKTIME ((8 * HOUR) + FREETIME) /// PR-DOBA
#define FREETIME_TIMEOUT (4 * HOUR) /// PRES-PRAC

#define LIFT_SPEED 8


InputParser *input;

Store patro2("Sklad - Druhe patro", 30);
Store patro1("Sklad - Prvni patro", 30);
Store patro0("Sklad - Prizemi", 30);

Facility vozik("Vozik pro presun palet [sec]");

Facility vytah("Vytah pro presun palet [sec]");
char vytah_patro = 0;

Histogram vychystaneZakazky("Pocty zpracovanych zakazek [n/hod]", 0, 1.f, WORKDAY / HOUR);
Histogram prestavky("Pocet prestavek [n/hod]", 0, 1.f, WORKDAY / HOUR);
Histogram presunyPalet("Presuny palet [min]", 0, 1.f, 15);
Histogram casyZakazek("Casy zakazek [min]", 0, 5.f, 12);
Histogram casyZakazekAlt("Casy zakazek (ALT) [min]", 0, 5.f, 12);

Stat vychystavani_patro2("Casy zakazek - 2. patro [min]");
Stat vychystavani_patro1("Casy zakazek - 1. patro [min]");
Stat vychystavani_patro0("Casy zakazek - prizemi [min]");

bool SIM_vytah = false;
double zacatekDne = 0;

class Pracovnik : public Process
{
	double _zacatekPrace;
	double _konecPrace;

	double _zacatekPrestavky;
	bool _melPrestavku = false;

	std::function<void()> P2PresunNaP1 = [&]() {
		if (SIM_vytah)
		{
			//	Presun pomoci vytahu
			Seize(vytah);
			{
				if (vytah_patro == 0)
					Wait(2 * LIFT_SPEED * SECOND);
				else if (vytah_patro == 1)
					Wait(LIFT_SPEED * SECOND);
				Wait(LIFT_SPEED * SECOND);
				Release(vytah);
			}
		}
		else
		{
			//	Presun pomoci vysokozdvizneho voziku
			if (Uniform(1, 10) > 8) ///	P2-VO-N
			{
				///	P2-VO-NH
				Wait(Uniform(30 * SECOND, 2 * MINUTE)); ///	P2-VO-H
			}
			else; ///	P2-VO-OK

			///	PR-P21-VS
			Seize(vozik); ///	P2-VO

			///	PR-P21
			{
				Wait(Uniform(30 * SECOND, 1 * MINUTE));
				Release(vozik);
			} ///	P2-VO-U
		}
	};

	std::function<void()> P1PresunNaP0 = [&]() {
		if (SIM_vytah)
		{
			//	Presun pomoci vytahu
			Seize(vytah);
			{
				if (vytah_patro != 1)
					Wait(LIFT_SPEED * SECOND);
				Wait(LIFT_SPEED * SECOND);
				Release(vytah);
			}
		}
		else
		{
			//	Presun pomoci vysokozdvizneho voziku
			if (Uniform(1, 10) > 8) ///	P1-VO-N
			{
				///	P1-VO-NH
				Wait(Uniform(30 * SECOND, 2 * MINUTE)); ///	P1-VO-H
			} else; ///	P1-VO-OK

			///	PR-P10-VS
			Seize(vozik); ///	P1-VO

			///	PR-P10
			{
				Wait(Uniform(30 * SECOND, 1 * MINUTE));
				Release(vozik);
			} ///	P1-VO-U
		}
	};

	void Behavior() override
	{
		do
		{
			///	PR-V
			Wait(1 * MINUTE); ///	ZAK

			if (_konecPrace < Time)
				Terminate(); ///	PR-ODCH
			else if (!_melPrestavku && _zacatekPrestavky < Time) ///	PRES-VS
			{
				prestavky((Time - zacatekDne) / float(HOUR));
				Wait(FREETIME); ///	PRES
				_melPrestavku = true;
			}

			///	PR-ZAK
			auto zacatekZakazky = Time;
			auto casPresunu = 0.0;
			auto vyberPatra = Uniform(1, 100);
			auto cas = Normal(30 * MINUTE, 6 * MINUTE) / 3.f;

			auto PracujNaPatre0 = [&](){
				Enter(patro0);

				///	PR-P0
				{
					Wait(cas);
					vychystavani_patro0(cas / float(MINUTE));
					Leave(patro0);
				} ///	PRAC-P0

				///	PR-P0-H
			};
			auto PracujNaPatre1 = [&](){
				Enter(patro1);

				///	PR-P1
				{
					Wait(cas);
					vychystavani_patro1(cas / float(MINUTE));
					Leave(patro1);
				} ///	PRAC-P1

				///	PR-P1-H
				auto zacatekPresunu = Time;
				P1PresunNaP0();
				casPresunu+= Time - zacatekPresunu;

				///	PR-P0-VS
				PracujNaPatre0(); ///	P0-VS
			};
			auto PracujNaPatre2 = [&](){
				Enter(patro2);

				///	PR-P2
				{
					Wait(cas);
					vychystavani_patro2(cas / float(MINUTE));
					Leave(patro2);
				} ///	PRAC-P2

				///	PR-P2-H
				auto zacatekPresunu = Time;
				P2PresunNaP1();
				casPresunu+= Time - zacatekPresunu;

				///	PR-P2-VS
				PracujNaPatre1(); ///	P2-VS
			};

			if (vyberPatra <= 75)
			{
				///	ZAK-P2, 75% zakazek zacina v 2. patre
				PracujNaPatre2();
			}
			else if (vyberPatra <= 95)
			{
				///	ZAK-P1, 20% zakazek zacina v 1. patre
				PracujNaPatre1();
			}
			else
			{
				///	ZAK-P0, 5% zakazek zacina v prizemi
				PracujNaPatre0();
			}

			///	PR-P0-H
			vychystaneZakazky((Time - zacatekDne) / float(HOUR));
			casyZakazek((Time - zacatekZakazky) / float(MINUTE));
			presunyPalet(casPresunu / float(MINUTE));
		}
		while (Time > 0);
	}

public:
	Pracovnik():
		_zacatekPrace(Time),
		_konecPrace(Time + WORKTIME),
		_zacatekPrestavky(Time + FREETIME_TIMEOUT)
	{
		Activate();
	}
};

class ZacatekDne : public Event
{
	void Behavior() override
	{
		zacatekDne = Time;
		Activate(Time + 24 * HOUR);
	}

public:
	explicit ZacatekDne()
	{
		Activate();
	}
};

class Prichod : public Event
{
	double _casSpusteni;
	int _pocetPracovniku;

	void Behavior() override
	{
		for (int idx = 0; idx < _pocetPracovniku; idx++) ///	PRCH
		{
			(new Pracovnik)->Activate(Time + Uniform(0 * MINUTE, 10 * MINUTE));
			//	TODO: Odchody?
		}
		Activate(Time + 24 * HOUR);
	}

public:
	explicit Prichod(int pocetPracovniku):
		_pocetPracovniku(pocetPracovniku),
		_casSpusteni(Time)
	{
		Activate();
	}
};

///
/// @return exit status code
///
int main(int argc, char **argv)
{
	//================================================================dd==
	//	INICIALIZACE SIMULACE
	//================================================================dd==
	input = new InputParser(argc, argv);
	SIM_vytah = input->cmdOptionExists("--vytah");

	auto pracovniku_rano = 8;
	if (input->cmdOptionExists("--prac-rano")) pracovniku_rano = std::stoi(input->getCmdOption("--prac-rano"));

	auto pracovniku_dopoledne = 12;
	if (input->cmdOptionExists("--prac-dopol")) pracovniku_rano = std::stoi(input->getCmdOption("--prac-dopol"));

	auto dni = 261;
	if (input->cmdOptionExists("--dni")) dni = std::stoi(input->getCmdOption("--dni"));


	//================================================================dd==
	//	INICIALIZACE SIMLIB
	//================================================================dd==
	//SetOutput("vysledky");
	Init(0, dni * (WORKDAY + 10 * HOUR));


	//================================================================dd==
	//	SIMULACE
	//================================================================dd==
	Print(" Sklad --- Model vicepatroveho skladu\n");
	(new ZacatekDne)->Activate();
	// 6:00
	(new Prichod(pracovniku_rano))->Activate();
	// 9:00
	(new Prichod(pracovniku_dopoledne))->Activate(3 * HOUR);

	// Spuštění simulace
	Run();

	// Výsledky
	//	TODO: Results
	if (SIM_vytah)
	{
		vytah.Output();
		presunyPalet.Output();
	}
	else
	{
		vozik.Output();
		presunyPalet.Output();
	}

	Print("\n================================================================dd==\n");
	Print("\t\tZakazky - statistiky v patrech\n");
	vychystavani_patro2.Output();
	patro2.Output();
	vychystavani_patro1.Output();
	patro1.Output();
	vychystavani_patro0.Output();
	patro0.Output();

	Print("\n================================================================dd==\n");
	Print("\t\tZakazky komplet - agregace\n");
	vychystaneZakazky.Output();
	casyZakazek.Output();
	casyZakazekAlt.Output();
	prestavky.Output();

	Print("\n");
	SIMLIB_statistics.Output();

	return 0;
}