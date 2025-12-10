using System.ComponentModel.DataAnnotations;

namespace SiteWeb.Models
{
    public class Nichoir
    {
        public int Id { get; set; }

        [Required, MaxLength(80)]
        public string Nom { get; set; } = "";

        [Required, MaxLength(64)]
        public string ChipId { get; set; } = "";

        [MaxLength(140)]
        public string? Emplacement { get; set; }

        public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    }
}
