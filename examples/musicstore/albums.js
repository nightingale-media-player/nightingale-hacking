/*
 * Array to represent track meta data. This would likely come from a database.
 */
var albums = [
	{
	id: "some-unique-key1",
	url: "http://mirrors.creativecommons.org/ccmixter/contrib/Wired/Beastie%20Boys%20-%20Now%20Get%20Busy.mp3", 
	previewUrl: MusicStore.fullUrl("media/01%20Now%20Get%20Busy.mp3"), 
	name: "Now Get Busy", // Track name 
	artist: "The Beastie Boys", // Artist
	album: "The Wired CD", // Album 
	genre: "Compilation", // Genre
	duration: MusicStore.toMicroseconds(0,2,22), // Duration
	number: 1, // Track Nb
	price: 0.99
	},
	{
	id: "some-unique-key2",
	url: "http://mirrors.creativecommons.org/ccmixter/contrib/Wired/David%20Byrne%20-%20My%20Fair%20Lady.mp3", 
	previewUrl: MusicStore.fullUrl("media/02%20My%20Fair%20Lady.mp3"), 
	name: "My Fair Lady", // Track name 
	artist: "David Byrne", // Artist
	album: "The Wired CD", // Album 
	genre: "Compilation", // Genre
	duration: MusicStore.toMicroseconds(0,3,28), // Duration
	number: 2, // Track Nb
	price: 0.99
	},
	{
	id: "some-unique-key3",		
	url: "http://mirrors.creativecommons.org/ccmixter/contrib/Wired/Gilberto%20Gil%20-%20Oslodum.mp3", 
	previewUrl: MusicStore.fullUrl("media/06%20Oslodum.mp3"), 
	name: "Oslodum", // Track name 
	artist: "Gilberto Gil", // Artist
	album: "The Wired CD", // Album 
	genre: "Compilation", // Genre
	duration: MusicStore.toMicroseconds(0,3,58), // Duration
	number: 6, // Track Nb
	price: 0.99
	},
	{
	id: "some-unique-key4",	
	url: "http://mirrors.creativecommons.org/ccmixter/contrib/Wired/Thievery%20Corporation%20-%20Dc%203000.mp3", 
	previewUrl: MusicStore.fullUrl("media/08%20Dc%203000.mp3"), 
	name: "DC 3000", // Track name 
	artist: "Thievery Corporation", // Artist
	album: "The Wired CD", // Album 
	genre: "Compilation", // Genre
	duration: MusicStore.toMicroseconds(0,4,28), // Duration
	number: 8, // Track Nb
	price: 0.99
	}
]