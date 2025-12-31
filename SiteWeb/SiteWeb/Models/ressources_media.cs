using System.ComponentModel.DataAnnotations;

namespace SiteWeb.Models
{
    public class ressources_media
    {
        public long Id { get; set; }

        public int NichoirId { get; set; }

        [Required, MaxLength(16)]
        public string Type { get; set; } = "image"; // image/video/autre

        [Required, MaxLength(255)]
        public string Chemin { get; set; } = ""; // ex: "captures/2025-12-04/xxx.jpg"

        public DateTime CapturedAt { get; set; } = DateTime.UtcNow;

        [MaxLength(16)]
        public string? Trigger { get; set; } // SCHEDULED / PIR / MANUAL

        public int? AlbumId { get; set; }
    }
}
