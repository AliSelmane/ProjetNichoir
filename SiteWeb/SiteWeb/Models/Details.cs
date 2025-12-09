namespace SiteWeb.Models
{
    public class Details
    {
        public Nichoir Nichoir { get; set; } = new Nichoir();
        public List<RessourceMedia> Medias { get; set; } = new List<RessourceMedia>();
    }
}
