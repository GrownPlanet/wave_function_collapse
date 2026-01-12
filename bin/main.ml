type color = { r : int; g : int; b : int }
type bitmap = { width : int; height : int; data : color array array }

let parse_input channel =
  let ib = Scanf.Scanning.from_channel channel in
  Scanf.bscanf ib " %s %d %d %d" (fun version width height color_space ->
      if version <> "P3" || color_space <> 255 then
        failwith "invalid header in the ppm file"
      else
        let data =
          Array.init_matrix width height (fun _ _ ->
              Scanf.bscanf ib " %d %d %d" (fun r b g -> { r; g; b }))
        in
        { width; height; data })

module PointSet = Set.Make (struct
  type t = int * int
  let compare = compare
end)

type neighbors = {
  up : PointSet.t;
  down : PointSet.t;
  left : PointSet.t;
  right : PointSet.t;
}

type direction = Up | Down | Left | Right

let opposite_dir dir =
  match dir with
  | Up -> Down
  | Down -> Up
  | Left -> Right
  | Right -> Left

let get_neighbor dir neighbors =
  match dir with
  | Up -> neighbors.up
  | Down -> neighbors.down
  | Left -> neighbors.left
  | Right -> neighbors.right

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
        if
          x < 0 || y < 0
          || x >= image.width - tile_size
          || y >= image.height - tile_size
        then None
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
    Printf.printf "collapsing %d %d\n" x y;
    let pos, changed =
      match tiles.(y).(x) with
      | Any -> (rand_array possibilities, true)
      | Constrained pos -> (rand_array pos, Array.length pos > 1)
      | Collapsed pos -> (pos, false)
    in
    tiles.(y).(x) <- Collapsed pos;
    changed
  in
  let get_neighbors x y =
    [ (Up, x, y - 1); (Down, x, y + 1); (Left, x - 1, y); (Right, x + 1, y) ]
    |> List.filter_map (fun (d, x, y) ->
        if x < 0 || y < 0 || x >= out_width || y >= out_height then None
        else Some (d, x, y))
  in
  let pos_in_dir d x y =
    let pos = 
      match tiles.(y).(x) with
      | Constrained c -> c
      | Collapsed c -> [| c |]
      | Any -> possibilities
    in
    pos
    |> Array.map
        (fun pt -> get_neighbor d (Hashtbl.find patterns.patterns pt))
    |> Array.fold_left (PointSet.union) (PointSet.empty)
  in
  let cell_equal_ps cell ps =
    let c =
      match cell with
      | Constrained c -> c
      | Collapsed c -> [| c |]
      | Any -> possibilities
    in
    if ps = PointSet.of_list (Array.to_list c) then
      true
    else
      false
  in
  let collapse_neighbors point =
    let rec aux x y =
      let possibilities =
        get_neighbors x y
        |> List.map (fun (d, x, y) -> pos_in_dir (opposite_dir d) x y)
        |> List.fold_left PointSet.inter PointSet.empty
      in
      if cell_equal_ps tiles.(y).(x) possibilities then
        false
      else (
        match tiles.(y).(x) with
        | Collapsed _ -> failwith "impossible path"
        | _ -> tiles.(y).(x) <- Constrained (Array.of_list (PointSet.to_list possibilities));
        get_neighbors x y |> List.iter (fun (_, x, y) -> let _ = aux x y in ());
        true)
    in
    let x, y = point in
    get_neighbors x y
    |> List.filter_map (fun (_, x, y) -> if aux x y then Some (x, y) else None)
  in
  let rec solve to_visit =
    if PointSet.is_empty to_visit then tiles
    else
      let x, y = PointSet.choose to_visit in
      let to_visit = PointSet.remove (x, y) to_visit in
      let changed = collapse_point x y in
      if changed then
        let neighbors = collapse_neighbors (x, y) |> List.to_seq in
        PointSet.add_seq neighbors to_visit |> solve
      else solve to_visit
  in
  let image_of_tiles tiles =
    let width = out_width * patterns.tile_size in
    let height = out_height * patterns.tile_size in
    let data =
      Array.init_matrix width height (fun x y ->
          let x1, y1 = (x / patterns.tile_size, y / patterns.tile_size) in
          let x2, y2 = (x mod patterns.tile_size, y mod patterns.tile_size) in
          let ix, iy =
            match tiles.(y1).(x1) with
            | Collapsed p -> p
            | _ -> failwith "something went wrong"
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
  let image = In_channel.with_open_text "input.ppm" parse_input in
  let patterns = extract_patterns image tile_size in
  let output = generate_image patterns 10 10 in
  write_image "out.ppm" output
