using System.ComponentModel.DataAnnotations.Schema;

namespace SiteWeb.Models
{
    [NotMapped]
    public class Details
    {
        public int Id { get; set; }
        public nichoirs Nichoir { get; set; } = new nichoirs();
        public List<ressources_media> Medias { get; set; } = new List<ressources_media>();

        public List<Album> Albums { get; set; } = new List<Album> ();
    }
}
