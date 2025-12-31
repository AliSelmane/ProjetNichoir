using System;

namespace SiteWeb.Models
{
    public class Album
    {
        public int Id { get; set; }

        public string Nom { get; set; }

        public int NichoirId { get; set; }

        public DateTime CreatedAt { get; set; }
    }
}
