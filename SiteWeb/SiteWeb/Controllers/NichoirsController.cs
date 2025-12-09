using Microsoft.AspNetCore.Mvc;
using SiteWeb.Models;

namespace SiteWeb.Controllers
{
    public class NichoirsController : Controller
    {
        // ✅ Stock en mémoire (DEMO) : disparaît au redémarrage
        private static List<Nichoir> _nichoirs = new List<Nichoir>
        {
            new Nichoir { Id = 1,  Nom = "Nichoir Jardin Nord",      ChipId = "ESP32-JN-8F21A3C7", Emplacement = "Jardin (Nord)", CreatedAt = DateTime.UtcNow.AddDays(-34) },
            new Nichoir { Id = 2,  Nom = "Nichoir Balcon",           ChipId = "ESP32-BC-12D9F0AA", Emplacement = "Balcon",        CreatedAt = DateTime.UtcNow.AddDays(-12) },
            new Nichoir { Id = 3,  Nom = "Nichoir Verger",           ChipId = "ESP32-VG-44CC01B2", Emplacement = "Verger",        CreatedAt = DateTime.UtcNow.AddDays(-58) },
            new Nichoir { Id = 4,  Nom = "Nichoir Serre",            ChipId = "ESP32-SR-9A0B11C8", Emplacement = "Serre",         CreatedAt = DateTime.UtcNow.AddDays(-21) },
            new Nichoir { Id = 5,  Nom = "Nichoir Cabane",           ChipId = "ESP32-CB-77E2D91F", Emplacement = "Cabane",        CreatedAt = DateTime.UtcNow.AddDays(-7)  },
            new Nichoir { Id = 6,  Nom = "Nichoir Étang",            ChipId = "ESP32-ET-1C3A8B9D", Emplacement = "Près de l'étang",CreatedAt = DateTime.UtcNow.AddDays(-90) },
            new Nichoir { Id = 7,  Nom = "Nichoir Haie Sud",         ChipId = "ESP32-HS-0FEE10A4", Emplacement = "Haie (Sud)",    CreatedAt = DateTime.UtcNow.AddDays(-15) },
            new Nichoir { Id = 8,  Nom = "Nichoir Atelier",          ChipId = "ESP32-AT-55B0E2D1", Emplacement = "Atelier",       CreatedAt = DateTime.UtcNow.AddDays(-5)  },
            new Nichoir { Id = 9,  Nom = "Nichoir Parking",          ChipId = "ESP32-PK-2B7D4C90", Emplacement = "Parking",       CreatedAt = DateTime.UtcNow.AddDays(-25) },
            new Nichoir { Id = 10, Nom = "Nichoir Jardin Est",       ChipId = "ESP32-JE-6A1C9D33", Emplacement = "Jardin (Est)",  CreatedAt = DateTime.UtcNow.AddDays(-40) },

            new Nichoir { Id = 11, Nom = "Nichoir Bois Clairière",   ChipId = "ESP32-BC-3D9A7F21", Emplacement = "Bois - clairière",CreatedAt = DateTime.UtcNow.AddDays(-120) },
            new Nichoir { Id = 12, Nom = "Nichoir Allée",            ChipId = "ESP32-AL-8C0B2E7A", Emplacement = "Allée",         CreatedAt = DateTime.UtcNow.AddDays(-18)  },
            new Nichoir { Id = 13, Nom = "Nichoir Potager",          ChipId = "ESP32-PT-11AA77CC", Emplacement = "Potager",       CreatedAt = DateTime.UtcNow.AddDays(-62)  },
            new Nichoir { Id = 14, Nom = "Nichoir Arbre #1",         ChipId = "ESP32-AR-0A9F3B12", Emplacement = "Chêne #1",      CreatedAt = DateTime.UtcNow.AddDays(-200) },
            new Nichoir { Id = 15, Nom = "Nichoir Arbre #2",         ChipId = "ESP32-AR-0A9F3B13", Emplacement = "Chêne #2",      CreatedAt = DateTime.UtcNow.AddDays(-180) },
            new Nichoir { Id = 16, Nom = "Nichoir Hauteur",          ChipId = "ESP32-HT-9988ABCD", Emplacement = "Mât (3m)",      CreatedAt = DateTime.UtcNow.AddDays(-33)  },
            new Nichoir { Id = 17, Nom = "Nichoir Terrasse",         ChipId = "ESP32-TR-7766DCBA", Emplacement = "Terrasse",      CreatedAt = DateTime.UtcNow.AddDays(-9)   },
            new Nichoir { Id = 18, Nom = "Nichoir Toit Garage",      ChipId = "ESP32-TG-AB12CD34", Emplacement = "Toit garage",   CreatedAt = DateTime.UtcNow.AddDays(-14)  },
            new Nichoir { Id = 19, Nom = "Nichoir Pignon",           ChipId = "ESP32-PG-CC44AA11", Emplacement = "Pignon maison", CreatedAt = DateTime.UtcNow.AddDays(-70)  },
            new Nichoir { Id = 20, Nom = "Nichoir Cour",             ChipId = "ESP32-CR-1F2E3D4C", Emplacement = "Cour",          CreatedAt = DateTime.UtcNow.AddDays(-4)   },

            new Nichoir { Id = 21, Nom = "Nichoir Prototype A",      ChipId = "ESP32-PA-0001DEMO", Emplacement = null,            CreatedAt = DateTime.UtcNow.AddDays(-2)   },
            new Nichoir { Id = 22, Nom = "Nichoir Prototype B",      ChipId = "ESP32-PB-0002DEMO", Emplacement = "",              CreatedAt = DateTime.UtcNow.AddDays(-1)   },
            new Nichoir { Id = 23, Nom = "Nichoir Démo Salon",       ChipId = "ESP32-DS-90FF11EE", Emplacement = "Salon (démo)",  CreatedAt = DateTime.UtcNow.AddDays(-3)   },
            new Nichoir { Id = 24, Nom = "Nichoir Campus HEPL",      ChipId = "ESP32-HE-1849BEEF", Emplacement = "Campus",        CreatedAt = DateTime.UtcNow.AddDays(-28)  },
            new Nichoir { Id = 25, Nom = "Nichoir Test Long Range",  ChipId = "ESP32-LR-5EEDC0DE", Emplacement = "Fond du jardin",CreatedAt = DateTime.UtcNow.AddDays(-45)  },
            new Nichoir { Id = 26, Nom = "Nichoir Vigne",            ChipId = "ESP32-VN-ABCDEF12", Emplacement = "Vigne",         CreatedAt = DateTime.UtcNow.AddDays(-88)  },
            new Nichoir { Id = 27, Nom = "Nichoir Prairie",          ChipId = "ESP32-PR-1010A0A0", Emplacement = "Prairie",       CreatedAt = DateTime.UtcNow.AddDays(-110) },
            new Nichoir { Id = 28, Nom = "Nichoir Bordure",          ChipId = "ESP32-BD-7F7F7F7F", Emplacement = "Bordure",       CreatedAt = DateTime.UtcNow.AddDays(-16)  },
            new Nichoir { Id = 29, Nom = "Nichoir Arbre Fruitier",   ChipId = "ESP32-AF-2C2C2C2C", Emplacement = "Pommier",       CreatedAt = DateTime.UtcNow.AddDays(-150) },
            new Nichoir { Id = 30, Nom = "Nichoir Zone Ombre",       ChipId = "ESP32-ZO-1337BADA", Emplacement = "Zone ombragée", CreatedAt = DateTime.UtcNow.AddDays(-60)  },
        };

        // GET: /Nichoirs
        public IActionResult Index()
        {
            return View(_nichoirs);
        }

        // GET: /Nichoirs/Details/1
        public IActionResult Details(int id)
        {
            var n = _nichoirs.FirstOrDefault(x => x.Id == id);
            if (n == null) return NotFound();

            var vm = new Details
            {
                Nichoir = n,
                Medias = new List<RessourceMedia>
                {
                    new RessourceMedia { Id=1, NichoirId=id, Type="image", Chemin="images/demo1.jpg", CapturedAt=DateTime.UtcNow, Trigger="SCHEDULED" },
                    new RessourceMedia { Id=2, NichoirId=id, Type="image", Chemin="images/demo2.jpg", CapturedAt=DateTime.UtcNow, Trigger="PIR" }
                }
            };

            return View(vm);
        }

        public IActionResult Create() => View();
        public IActionResult Edit(int id) => View();

        // ✅ OPTIONNEL : page Delete (si tu ne l’utilises pas, tu peux supprimer cette action)
        public IActionResult Delete(int id) => View();

        // ✅ POST : suppression réelle (DEMO) après confirmation du modal
        [HttpPost]
        [ValidateAntiForgeryToken]
        [HttpPost]
        public IActionResult DeleteConfirmed(int id)
        {
            var cible = _nichoirs.FirstOrDefault(x => x.Id == id);
            if (cible != null)
                _nichoirs.Remove(cible);

            return RedirectToAction(nameof(Index));
        }

    }
}
