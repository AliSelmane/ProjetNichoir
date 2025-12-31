using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using SiteWeb.Models;
using System;
using System.Linq;

namespace SiteWeb.Controllers
{
    public class NichoirsController : Controller
    {
        private readonly ApplicationDbContext _db;

        public NichoirsController(ApplicationDbContext db)
        {
            _db = db;
        }

        // ==========================================
        // 1. LISTE DES NICHOIRS
        // ==========================================
        public IActionResult Index()
        {
            // .AsNoTracking() est plus performant pour l'affichage simple
            var nichoirs = _db.Nichoirs.AsNoTracking().ToList();
            return View(nichoirs);
        }

        // ==========================================
        // 2. DETAILS DU NICHOIR
        // ==========================================
        public IActionResult Details(int id)
        {
            var nichoir = _db.Nichoirs.FirstOrDefault(n => n.Id == id);
            if (nichoir == null) return NotFound();

            var albums = _db.Albums
                .Where(a => a.NichoirId == id)
                .OrderBy(a => a.Nom)
                .ToList();

            var medias = _db.RessourcesMedia
                .Where(m => m.NichoirId == id)
                .ToList();

            // 🔥 LOGIQUE DE NORMALISATION : Rattachement automatique à l'album "Général"
            var generalAlbum = albums.FirstOrDefault(a => a.Nom == "Général");
            if (generalAlbum != null)
            {
                // On vérifie les médias sans album (AlbumId est null ou 0)
                var mediasSansAlbum = medias.Where(m => m.AlbumId == null || m.AlbumId == 0).ToList();
                if (mediasSansAlbum.Any())
                {
                    foreach (var m in mediasSansAlbum)
                    {
                        m.AlbumId = generalAlbum.Id;
                        _db.Entry(m).State = EntityState.Modified; // Force le suivi de modification
                    }
                    _db.SaveChanges();
                }
            }

            // Utilisation de votre classe de ViewModel 'Details'
            var vm = new Details
            {
                Nichoir = nichoir,
                Albums = albums,
                Medias = medias
            };

            return View(vm);
        }

        // ==========================================
        // 3. DEPLACER IMAGE DANS UN ALBUM
        // ==========================================
        [HttpPost]
        public IActionResult MoveToAlbum(long mediaId, int albumId) // mediaId est long pour correspondre à la DB
        {
            var media = _db.RessourcesMedia.Find(mediaId);
            if (media == null) return NotFound("Image introuvable");

            // Vérifier si l'album existe
            var albumExists = _db.Albums.Any(a => a.Id == albumId);
            if (!albumExists) return BadRequest("L'album cible n'existe pas");

            media.AlbumId = albumId;

            // On force l'état pour garantir que l'UPDATE SQL soit généré
            _db.Entry(media).State = EntityState.Modified;

            try
            {
                _db.SaveChanges();
                return Ok(); // Réponse pour un appel AJAX
            }
            catch (Exception ex)
            {
                return BadRequest("Erreur DB : " + ex.Message);
            }
        }

        // ==========================================
        // 4. CREER UN NOUVEL ALBUM
        // ==========================================
        [HttpPost]
        public IActionResult CreateAlbum(int nichoirId, string name)
        {
            if (string.IsNullOrWhiteSpace(name)) return BadRequest("Nom d'album requis");

            var album = new Album
            {
                Nom = name,
                NichoirId = nichoirId,
                CreatedAt = DateTime.Now
            };

            _db.Albums.Add(album);
            _db.SaveChanges();

            return Ok(album.Id);
        }

        // ==========================================
        // 5. BASCULER EN FAVORI (Toggle)
        // ==========================================
        [HttpPost]
        public IActionResult ToggleFavorite(long id) // id est long ici
        {
            int idalbumfavoris = _db.Set<Album>().FirstOrDefault(al => al.Nom.ToLower() == "favoris").Id;

            var ressourcemedia = _db.Set<ressources_media>().FirstOrDefault(al => al.Id == id);

            if (ressourcemedia != null)
            {
                ressourcemedia.AlbumId = idalbumfavoris;
                _db.Set<ressources_media>().Update(ressourcemedia);
                _db.SaveChanges();
            }



            return Ok();
        }

        // ==========================================
        // 6. SUPPRIMER UNE IMAGE
        // ==========================================
        [HttpPost]
        public IActionResult DeleteImage(long id) // id est long
        {
            var image = _db.RessourcesMedia.Find(id);
            if (image == null) return NotFound();

            int nichoirId = image.NichoirId;

            _db.RessourcesMedia.Remove(image);
            _db.SaveChanges();

            return RedirectToAction("Details", new { id = nichoirId });
        }

        // ==========================================
        // 7. SUPPRIMER UN NICHOIR (Et ses contenus)
        // ==========================================
        [HttpPost]
        public IActionResult DeleteNichoir(int id)
        {
            var nichoir = _db.Nichoirs.Find(id);
            if (nichoir == null) return NotFound();

            // Suppression manuelle en cascade si nécessaire
            var medias = _db.RessourcesMedia.Where(m => m.NichoirId == id);
            var albums = _db.Albums.Where(a => a.NichoirId == id);

            _db.RessourcesMedia.RemoveRange(medias);
            _db.Albums.RemoveRange(albums);
            _db.Nichoirs.Remove(nichoir);

            _db.SaveChanges();

            return RedirectToAction("Index");
        }
    }
}