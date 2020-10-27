for use_neumann_bc in false true; do
for improve_mesh in true false; do
for method in splines stl; do
for refinement in 1 2 3; do
  
  # clear output directory
  rm -rf out
  echo -n "$(date): " >> calls.txt 
  
  # run program
  echo $method $refinement $improve_mesh $use_neumann_bc
  mpirun -n 64 ./generate ../settings_generate.py 
    --input_filename_or_splines_or_stl $method \
    --refinement_factor $refinement \
    --improve_mesh $improve_mesh \
    --use_neumann_bc $use_neumann_bc | tee -a calls.txt
  
  # run mesh_evaluate_quality on created files
  for file in `ls -rt *.bin* | head -n 2`; do
    mesh_evaluate_quality.py $file
  done

  # rename output directory
  mv out out_${method}_${refinement}_${improve_mesh}_${use_neumann_bc}
  
done
done
done
done
