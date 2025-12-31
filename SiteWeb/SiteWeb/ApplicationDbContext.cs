using Microsoft.EntityFrameworkCore;
using SiteWeb.Models;

public class ApplicationDbContext : DbContext
{
    public ApplicationDbContext(DbContextOptions<ApplicationDbContext> options)
        : base(options)
    {
    }

    public DbSet<nichoirs> Nichoirs { get; set; }
    public DbSet<ressources_media> RessourcesMedia { get; set; }
    public DbSet<Details> Details { get; set; }
    public DbSet<Album> Albums { get; set; }



    protected override void OnModelCreating(ModelBuilder modelBuilder)
    {
        base.OnModelCreating(modelBuilder);

        // ---------------------------
        // Table : Nichoirs
        // ---------------------------
        modelBuilder.Entity<nichoirs>()
            .ToTable("nichoirs")
            .HasKey(n => n.Id);

        modelBuilder.Entity<nichoirs>()
            .Property(n => n.Nom)
            .IsRequired()
            .HasMaxLength(80);

        modelBuilder.Entity<nichoirs>()
            .Property(n => n.ChipId)
            .IsRequired()
            .HasMaxLength(64);

        modelBuilder.Entity<nichoirs>()
            .Property(n => n.Emplacement)
            .HasMaxLength(140);

        // ---------------------------
        // Table : RessourceMedia
        // ---------------------------
        modelBuilder.Entity<ressources_media>()
            .ToTable("ressources_media")
            .HasKey(r => r.Id);

        modelBuilder.Entity<ressources_media>()
            .Property(r => r.Type)
            .IsRequired()
            .HasMaxLength(16);

        modelBuilder.Entity<ressources_media>()
            .Property(r => r.Chemin)
            .IsRequired()
            .HasMaxLength(255);

        modelBuilder.Entity<ressources_media>()
            .Property(r => r.Trigger)
            .HasMaxLength(16);

        // ---------------------------
        // Relation : 1 Nichoir → N RessourceMedia
        // ---------------------------
        modelBuilder.Entity<ressources_media>()
            .HasOne<nichoirs>()                        // un média appartient à un nichoir
            .WithMany()                               // un nichoir peut avoir plusieurs médias
            .HasForeignKey(r => r.NichoirId)          // FK dans RessourceMedia
            .OnDelete(DeleteBehavior.Cascade);        // si nichoir supprimé → supprime les médias

        // ---------------------------
        // Table : Albums
        // ---------------------------
        modelBuilder.Entity<Album>()
            .ToTable("albums")
            .HasKey(a => a.Id);

        modelBuilder.Entity<Album>()
            .Property(a => a.Nom)
            .IsRequired()
            .HasMaxLength(100);

        modelBuilder.Entity<Album>()
            .Property(a => a.CreatedAt)
            .IsRequired();

    }
}
