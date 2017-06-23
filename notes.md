API Notes

libcurl4-openssl-dev (in ubuntu)

// session append
for_each(blocks.begin(), blocks.end(), 
    [&token, &session, &file, &ttime, &took, &off](const char*& block) {
});

// API

// EP_UPLOAD_SESSION_START -----------------------------------------------------
// params:
// {
//     "close": false
// }
// returns:
// {
//     "session": "1234faaf0678bcde"
// }
// errors nothing!

// EP_UPLOAD_SESSION_APPEND ----------------------------------------------------
// params:
// {
//     "cursor": {
//         "session_id": "1234faaf0678bcde",
//         "offset": 0
//     },
//     "close": false
// }
// returns nothing!
// errors error_summary

// EP_UPLOAD_SESSION_FINISH ----------------------------------------------------
// params:
// {
//     "cursor": {
//         "session_id": "1234faaf0678bcde",
//         "offset": 0
//     },
//     "commit": {
//         "path": "/Homework/math/Matrices.txt",
//         "mode": "add",
//         "autorename": true,
//         "mute": false
//     }
// }
// returns:
// {
//     "name": "Prime_Numbers.txt",
//     "id": "id:a4ayc_80_OEAAAAAAAAAXw",
//     "client_modified": "2015-05-12T15:50:38Z",
//     "server_modified": "2015-05-12T15:50:38Z",
//     "rev": "a1c10ce0dd78",
//     "size": 7212,
//     "path_lower": "/homework/math/prime_numbers.txt",
//     "path_display": "/Homework/math/Prime_Numbers.txt",
//     "sharing_info": {
//         "read_only": true,
//         "parent_shared_folder_id": "84528192421",
//         "modified_by": "dbid:AAH4f99T0taONIb-OurWxbNQ6ywGRopQngc"
//     },
//     "property_groups": [
//         {
//             "template_id": "ptid:1a5n2i6d3OYEAAAAAAAAAYa",
//             "fields": [
//                 {
//                     "name": "Security Policy",
//                     "value": "Confidential"
//                 }
//             ]
//         }
//     ],
//     "has_explicit_shared_members": false,
//     "content_hash": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4648901b7852b855"
// }
// errors: error_summary