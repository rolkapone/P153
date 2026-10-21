# Table des clips de reference (donnees lues par scripts/fetch-clips.ps1 et tests/quality/*.ps1).
# Description humaine, licences et usage : docs/qa/clips.md. Rien ici n'est personnel.
# Sha256 : vide tant que le fichier n'a pas ete recupere une premiere fois ; fetch-clips.ps1 affiche alors
# la somme observee (a consigner ici et dans docs/qa/clips.md apres relecture). Ensuite elle est verifiee.
# Mode : Interp60 (60 -> 30 -> 60), NoInterp50, Align24 (24 -> 60, images alignees), Full (synthetique 60 i/s)
#        NoRef (archive du PO : pas de verite terrain, pas de VMAF)
# Start / Duration : secondes dans la source. Valeurs de depart [H] a fixer apres visionnage.
@{
  Hosts = @('download.blender.org', 'media.xiph.org', 'download.opencontent.netflix.com')   # liste blanche des hotes HTTPS
  Clips = @(
    @{ Id='BBB-FAST';  Kind='Download'; SourceId='BBB4K60'; Mode='Interp60';   Start=408.0;  Duration=10.0 }
    @{ Id='BBB-PAN';   Kind='Download'; SourceId='BBB4K60'; Mode='Interp60';   Start=5.0; Duration=10.0 }
    @{ Id='TOS-FACE';  Kind='Download'; SourceId='TOS4K';   Mode='Align24';    Start=316.0; Duration=10.0 }
    @{ Id='TOS-DARK';  Kind='Download'; SourceId='TOS4K';   Mode='Align24';    Start=153.0;  Duration=10.0 }
    @{ Id='CROWD-50';  Kind='Download'; SourceId='CROWD50'; Mode='NoInterp50'; Start=0.0;   Duration=10.0 }
    @{ Id='SOL-2D';    Kind='Download'; SourceId='SOL4K';   Mode='Align24';    Start=60.0;  Duration=10.0 }
    @{ Id='MERI-GRAIN';Kind='Download'; SourceId='MERI4K';  Mode='Align24';    Start=120.0; Duration=10.0 }
    @{ Id='SYN-TEXT';  Kind='Synthetic';                    Mode='Full';       Start=0.0;   Duration=10.0 }
    @{ Id='PO-RETRO';  Kind='PO';       Mode='NoRef';       Start=0.0; Duration=15.0 }
    @{ Id='PO-ARCH';   Kind='PO';       Mode='NoRef';       Start=0.0; Duration=15.0 }
  )
  Sources = @(
    # Url : emplacement officiel attendu, A CONFIRMER a la premiere recuperation (le script echoue proprement sur 404).
    @{ SourceId='BBB4K60'; FileName='bbb_sunflower_2160p_60fps_normal.mp4'; Archive='bbb_sunflower_2160p_60fps_normal.mp4.zip'; Sha256='8f1d53cf56a314be08206ae730546ef26db9fd0114b33ff2528f522bc3714d19';
       Url='https://download.blender.org/demo/movies/BBB/bbb_sunflower_2160p_60fps_normal.mp4.zip' }   # confirme 2026-10-21 (200 OK, 671868845 octets) ; Sha256 = celle de l'ARCHIVE zip
    @{ SourceId='TOS4K';   FileName='tearsofsteel_4k.mov'; Archive='tearsofsteel_4k.mov.zip'; Sha256='9c0b8df301d254e83d0046c530a71438bbfc42d2b7000366edde1596616bad05';
       Url='https://download.blender.org/demo/movies/ToS/tearsofsteel_4k.mov.zip' }   # confirme 2026-10-21 (200 OK, 6737593114 octets)
    @{ SourceId='CROWD50'; FileName='crowd_run_2160p50.y4m'; Sha256='';
       Url='https://media.xiph.org/video/derf/y4m/crowd_run_2160p50.y4m' }
    @{ SourceId='SOL4K';   FileName='SolLevante_4k.mov'; Sha256='';
       Url='' }   # a renseigner : emplacement exact Netflix Open Content a confirmer
    @{ SourceId='MERI4K';  FileName='Meridian_4k_sdr.mov'; Sha256='';
       Url='' }   # a renseigner : version SDR 4K requise (pas la version HDR PQ)
  )
}

