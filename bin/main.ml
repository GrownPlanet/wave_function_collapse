type color = { r : int; g : int; b : int }
type bitmap = { width : int; height : int; data : color array array }

let read_file filename = In_channel.(with_open_text filename input_lines)

let parse_data width data =
  let update_acc row_acc row_accl acc =
    if row_accl >= width then ([], 0, List.rev row_acc :: acc)
    else (row_acc, row_accl, acc)
  in
  let rec aux row_acc row_accl acc data =
    match data with
    | p :: r ->
        let row_acc', row_accl', acc =
          update_acc (p :: row_acc) (row_accl + 1) acc
        in
        aux row_acc' row_accl' acc r
    | [] -> List.rev acc |> List.map Array.of_list |> Array.of_list
  in
  aux [] 0 [] data

let parse_input data =
  match data with
  | version :: size :: color_space :: data ->
      if version <> "P3" || color_space <> "255" then
        invalid_arg "invalid header in the ppm file"
      else
        let width, height =
          try Scanf.sscanf size "%d %d" (fun w h -> (w, h))
          with End_of_file ->
            invalid_arg "invalid image dimensions in the ppm file"
        in
        let data =
          try
            data
            |> List.map (fun c ->
                Scanf.sscanf c "%d %d %d" (fun r b g -> { r; b; g }))
            |> parse_data width
          with End_of_file -> invalid_arg "invalid color in the ppm file"
        in
        if height > Array.length data then
          invalid_arg "not enough data in the ppm file"
        else { width; height; data }
  | _ -> invalid_arg "invalid ppm file"

module PointSet = Set.Make (struct
  type t = int * int

  let compare = compare
end)

type direction = Up | Down | Left | Right

type neighbors = {
  up : PointSet.t;
  down : PointSet.t;
  left : PointSet.t;
  right : PointSet.t;
}

type patterns = {
  image : bitmap;
  tile_size : int;
  patterns : (int * int, neighbors) Hashtbl.t;
}

let get_area x y image tile_size =
  List.init tile_size (fun yo ->
      List.init tile_size (fun xo -> image.data.(y + yo).(x + xo)))
  |> List.concat

let extract_patterns image tile_size =
  let indices = Hashtbl.create 256 in
  let patterns = Hashtbl.create 256 in
  let get_idx x y area =
    match Hashtbl.find_opt indices area with
    | Some (x, y) -> (x, y)
    | None ->
        Hashtbl.add indices area (x, y);
        (x, y)
  in
  let update_neighbor dir idx n =
    match dir with
    | Up -> { n with up = PointSet.add idx n.up }
    | Down -> { n with down = PointSet.add idx n.down }
    | Left -> { n with left = PointSet.add idx n.left }
    | Right -> { n with right = PointSet.add idx n.right }
  in
  let add_neighbor idx (dir, nx, ny, area) =
    let nidx = get_idx nx ny area in
    match Hashtbl.find_opt patterns idx with
    | Some n -> n |> update_neighbor dir nidx |> Hashtbl.replace patterns idx
    | None ->
        {
          up = PointSet.empty;
          down = PointSet.empty;
          left = PointSet.empty;
          right = PointSet.empty;
        }
        |> update_neighbor dir nidx |> Hashtbl.add patterns idx
  in
  let get_neighbors tile_size image x y =
    [
      (Up, x, y - tile_size);
      (Down, x, y + tile_size);
      (Left, x - tile_size, y);
      (Right, x + tile_size, y);
    ]
    |> List.filter_map (fun (d, x, y) ->
        if x < 0 || y < 0 || x >= image.width - 3 || y >= image.height - 3 then
          None
        else Some (d, x, y, get_area x y image tile_size))
  in
  for y = 0 to image.height - tile_size do
    for x = 0 to image.width - tile_size do
      let idx = get_idx x y (get_area x y image tile_size) in
      get_neighbors tile_size image x y |> List.iter (add_neighbor idx)
    done
  done;
  { image; tile_size; patterns }

type cell = Any | Constrained of (int * int) array | Collapsed of (int * int)

let generate_image patterns out_width out_height =
  let possibilities =
    PointSet.empty
    |> PointSet.add_seq
         (Hashtbl.to_seq patterns.patterns |> Seq.map (fun (k, _) -> k))
    |> PointSet.to_seq |> Array.of_seq
  in
  let tiles = Array.init_matrix out_width out_height (fun _ _ -> Any) in
  let rand_array a = a.(Random.int (Array.length a)) in
  let collapse_point x y =
    let pos, changed =
      match tiles.(y).(x) with
      | Any -> (rand_array possibilities, true)
      | Constrained pos -> (rand_array pos, Array.length pos > 1)
      | Collapsed pos -> (pos, false)
    in
    tiles.(y).(x) <- Collapsed pos;
    changed
  in
  (*
  let get_neighbors x y =
    [
      (x, y - 1);
      (x, y + 1);
      (x - 1, y);
      (x + 1, y);
    ]
    |> List.filter_map (fun (x, y) ->
        if x < 0 || y < 0 || x >= out_width || y >= out_height then None
        else Some (x, y))
  in
  *)
  let collapse_neighbors point =
    let rec aux point =
      let _ = point in
      ()
    in
    let _ = aux point in
    (* TODO; I was working here :) *)
    ()
  in
  let rec solve to_visit =
    if PointSet.is_empty to_visit then tiles
    else
      let x, y = PointSet.choose to_visit in
      let to_visit = PointSet.remove (x, y) to_visit in
      let changed = collapse_point x y in
      if changed then
        (collapse_neighbors (x, y); (* TODO: add neighbors to to_visit *)
        solve to_visit)
      else solve to_visit
  in
  let image_of_tiles tiles =
    let width = out_width * patterns.tile_size in
    let height = out_height * patterns.tile_size in
    let data =
      Array.init_matrix width height (fun x y ->
          let x1, y1 = (x / 3, y / 3) in
          let x2, y2 = (x mod 3, y mod 3) in
          let ix, iy =
            match tiles.(y1).(x1) with
            | Collapsed p -> p
            | _ -> invalid_arg "something went wrong"
          in
          patterns.image.data.(iy + y2).(ix + x2))
    in
    { data; width; height }
  in
  let ip = (Random.int out_width, Random.int out_height) in
  solve PointSet.(empty |> add ip) |> image_of_tiles

let write_image name image =
  Out_channel.with_open_text name (fun ch ->
      Printf.fprintf ch "P3\n%d %d\n255\n" image.width image.height;
      Array.iter
        (Array.iter (fun c -> Printf.fprintf ch "%d %d %d\n" c.r c.g c.b))
        image.data)

let () =
  Random.set_state (Random.State.make_self_init ());
  let tile_size = 3 in
  let image = parse_input (read_file "input.ppm") in
  let patterns = extract_patterns image tile_size in
  let output = generate_image patterns 10 10 in
  let _ = Constrained [||] in
  write_image "out.ppm" output
